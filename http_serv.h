#ifndef HTTP_SERV_H
#define HTTP_SERV_H

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "http_parser.h"

#define HTTP_PORT 80
#define POLL_TIME_S 5

extern CURRENT_STATE_T current_state;

typedef struct TCP_SERVER_T_ {
	struct tcp_pcb * server_pcb;
	bool complete;
	ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
	struct tcp_pcb * pcb;
	int sent_len;
	HTTP_REQUEST_PARSER_T * request_parser;
	HTTP_RESPONSE_COMPOSER_T * response_composer;
	ip_addr_t * gw;
} TCP_CONNECT_STATE_T;

TCP_SERVER_T * http_serv_state = NULL;

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T * con_state, struct tcp_pcb * client_pcb, err_t close_err) {
	if (client_pcb) {
		assert(con_state && con_state->pcb == client_pcb);
		tcp_arg(client_pcb, NULL);
		tcp_poll(client_pcb, NULL, 0);
		tcp_sent(client_pcb, NULL);
		tcp_recv(client_pcb, NULL);
		tcp_err(client_pcb, NULL);
		err_t err = tcp_close(client_pcb);
		if (err != ERR_OK) {
			DEBUG_printf("[HTTP] [ERR] Close failed %d, calling abort\n", err);
			tcp_abort(client_pcb);
			close_err = ERR_ABRT;
		}
		if (con_state) {
			delete_request_parser(con_state->request_parser);
			delete_response_composer(con_state->response_composer);
			free(con_state);
		}
		DEBUG_printf("[HTTP] [OK ] Finished and closed connection to client\n");
	}
	return close_err;
}

static void tcp_server_close(TCP_SERVER_T * state) {
	if (state->server_pcb) {
		tcp_arg(state->server_pcb, NULL);
		tcp_close(state->server_pcb);
		state->server_pcb = NULL;
	}
}

static err_t tcp_server_sent(void * arg, struct tcp_pcb * pcb, u16_t len) {
	TCP_CONNECT_STATE_T * con_state = (TCP_CONNECT_STATE_T *)arg;
	DEBUG_printf("[HTTP] [OK ] Sent %u\n", len);
	con_state->sent_len += len;
	if (con_state->sent_len >= con_state->response_composer->header_length + con_state->response_composer->content_length) {
		DEBUG_printf("[HTTP] [OK ] Sent done\n");
		return tcp_close_client_connection(con_state, pcb, ERR_OK);
	}
	return ERR_OK;
}

int send_response (TCP_CONNECT_STATE_T * con_state, struct tcp_pcb * pcb) {
	// Check result buffer size
	if (con_state->response_composer->content_length > sizeof(con_state->response_composer->body) - 1) {
		DEBUG_printf("[HTTP] [ERR] Too much result data %d\n", con_state->response_composer->content_length);
		return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
	}
	// Check header buffer size
	if (con_state->response_composer->header_length > sizeof(con_state->response_composer->header) - 1) {
		DEBUG_printf("[HTTP] [ERR] Too much header data %d\n", con_state->response_composer->header_length);
		return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
	}

	// Send the headers to the client
	con_state->sent_len = 0;
	err_t err = tcp_write(pcb, con_state->response_composer->header, con_state->response_composer->header_length, 0);
	if (err != ERR_OK) {
		DEBUG_printf("[HTTP] [ERR] Failed to write header data %d\n", err);
		return tcp_close_client_connection(con_state, pcb, err);
	}
	// Send the body to the client
	if (con_state->response_composer->content_length) {
		err = tcp_write(pcb, con_state->response_composer->body, con_state->response_composer->content_length, 0);
		if (err != ERR_OK) {
			DEBUG_printf("[HTTP] [ERR] Failed to write result data %d\n", err);
			return tcp_close_client_connection(con_state, pcb, err);
		}
	}
	return 0;
}

void handle_status_get (const char * request, const char * params, char * body, HTTP_RESPONSE_COMPOSER_T * response) {
	set_status(response, 200);
	set_type(response, "application/json");
	char response_body[128];
	snprintf(response_body, 128, "{volt: %f, temp: %f, power: %d}", current_state.voltage, current_state.tempC, current_state.power_state);
	set_body(response, response_body);
}

void handle_power_post (const char * request, const char * params, char * body, HTTP_RESPONSE_COMPOSER_T * response) {
	set_type(response, "application/json");
	if (strstr(body, "requested_state")) {
		int requested_power_state_int;
		int led_param = sscanf(body, "requested_state=%d", &requested_power_state_int);
		if (led_param) {
			if  (requested_power_state_int == 0 || requested_power_state_int == 1) {
				set_status(response, 200);
				bmc_power_handler((bool) requested_power_state_int);
				set_body(response, "{}");
			}
			else {
				set_status(response, 400);
				set_body(response, "{error: true, description: \"invalid requested state, must be 0 or 1\"}"); 
			}
		}
		else {
			set_status(response, 400);
			set_body(response, "{error: true, description: \"invalid requested state, must be 0 or 1\"}");
		}
	}
	else {
		set_status(response, 400);
		set_body(response, "{error: true, description: \"missing required parameter requested_state\"}");
	}
}

void handle_404_not_found (const char * request, const char * params, char * body, HTTP_RESPONSE_COMPOSER_T * response) {
	set_status(response, 404);
	set_type(response, "application/json");
	set_body(response, "");
}

err_t tcp_server_recv(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err) {
	TCP_CONNECT_STATE_T * con_state = (TCP_CONNECT_STATE_T *)arg;
	if (!p) {
		DEBUG_printf("[HTTP] [ERR] Client connection closed\n");
		return tcp_close_client_connection(con_state, pcb, ERR_OK);
	}
	assert(con_state && con_state->pcb == pcb);
	if (p->tot_len > 0) {
		DEBUG_printf("[HTTP] [OK ] Recieved %d err: %d\n", p->tot_len, err);

		char content[2048] = {0};
		pbuf_copy_partial(p, &content, 2048 - 1, 0);
		con_state->request_parser = new_request_parser(); 
		parse_http_request(con_state->request_parser, content, strlen(content));

		llhttp_method_t method = get_method(con_state->request_parser);
		char * url = get_url(con_state->request_parser);
		char * body = get_body(con_state->request_parser);
		
		// debug print request
		const char * method_name = llhttp_method_name(method);
		DEBUG_printf("[HTTP] [OK ] Request: %s %s %s\n", method_name, url, body);

		con_state->response_composer = new_response_composer();

		// parse request depending on method and request
		if (method == HTTP_GET && strncmp(url, "/status", sizeof("/status") - 1) == 0) {
			handle_status_get(url, NULL, body, con_state->response_composer);
		}
		else if (method == HTTP_POST && strncmp(url, "/power", sizeof("/power") - 1) == 0) {
			handle_power_post(url, NULL, body, con_state->response_composer);
		}
		else { // if not a registered path, return HTTP 404
			handle_404_not_found(url, NULL, body, con_state->response_composer);
		}

		compose_http_response(con_state->response_composer);

		// print result
		DEBUG_printf("[HTTP] [OK ] Result: %s %s\n", con_state->response_composer->header, con_state->response_composer->body);

		int err;
		if (err = send_response(con_state, pcb)) {
			DEBUG_printf("[HTTP] [ERR] Failure in send %d\n", err);
		}

		tcp_recved(pcb, p->tot_len);
	}
	pbuf_free(p);
	return ERR_OK;
}

static err_t tcp_server_poll(void * arg, struct tcp_pcb * pcb) {
	TCP_CONNECT_STATE_T * con_state = (TCP_CONNECT_STATE_T *)arg;
	return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void * arg, err_t err) {
	TCP_CONNECT_STATE_T * con_state = (TCP_CONNECT_STATE_T *)arg;
	if (err != ERR_ABRT) {
		DEBUG_printf("[HTTP] [ERR] Error %d\n", err);
		tcp_close_client_connection(con_state, con_state->pcb, err);
	}
}

static err_t tcp_server_accept(void * arg, struct tcp_pcb * client_pcb, err_t err) {
	TCP_SERVER_T * state = (TCP_SERVER_T *)arg;
	if (err != ERR_OK || client_pcb == NULL) {
		DEBUG_printf("[HTTP] [ERR] Failure accepting client connection\n");
		return ERR_VAL;
	}
	DEBUG_printf("[HTTP] [OK ] Client connected\n");

	// Create the state for the connection
	TCP_CONNECT_STATE_T * con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
	if (!con_state) {
		DEBUG_printf("[HTTP] [ERR] Failed to allocate connect state\n");
		return ERR_MEM;
	}
	con_state->pcb = client_pcb; // for checking
	con_state->gw = &state->gw;

	// setup connection to client
	tcp_arg(client_pcb, con_state);
	tcp_sent(client_pcb, tcp_server_sent);
	tcp_recv(client_pcb, tcp_server_recv);
	tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
	tcp_err(client_pcb, tcp_server_err);

	return ERR_OK;
}

static bool tcp_server_open(void * arg) {
	TCP_SERVER_T * state = (TCP_SERVER_T *)arg;
	DEBUG_printf("[HTTP] [OK ] Starting server at %s on port %d\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), HTTP_PORT);

	struct tcp_pcb * pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		DEBUG_printf("[HTTP] [ERR] Failed to create pcb\n");
		return false;
	}

	err_t err = tcp_bind(pcb, IP_ANY_TYPE, HTTP_PORT);
	if (err) {
		DEBUG_printf("[HTTP] [ERR] Failed to bind to port %d\n", HTTP_PORT);
		return false;
	}

	state->server_pcb = tcp_listen_with_backlog(pcb, 1);
	if (!state->server_pcb) {
		DEBUG_printf("[HTTP] [ERR] Failed to listen on port %d\n", HTTP_PORT);
		if (pcb) {
			tcp_close(pcb);
		}
		return false;
	}

	tcp_arg(state->server_pcb, state);
	tcp_accept(state->server_pcb, tcp_server_accept);

	return true;
}

int http_serv_init () {
	// init http parser
	if (http_parser_init()) {
		DEBUG_printf("[HTTP] [ERR] Failed to initialize http parser\n");
	}

	http_serv_state = calloc(1, sizeof(TCP_SERVER_T));
	if (!http_serv_state) {
		DEBUG_printf("[HTTP] [ERR] Failed to allocate state\n");
		return 1;
	}

	if (!tcp_server_open(http_serv_state)) {
		DEBUG_printf("[HTTP] [ERR] Failed to open server\n");
		return 1;
	}

	DEBUG_printf("[HTTP] [OK ] Sucessfully initialized http server\n");

	return 0;
}

int http_serv_deinit () {
	http_parser_deinit();
	tcp_server_close(http_serv_state);
	free(http_serv_state);
	http_serv_state = NULL;
	return 0;
}

#endif
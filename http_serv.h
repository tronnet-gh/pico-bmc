#ifndef HTTP_SERV_H
#define HTTP_SERV_H

#include <string.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define HTTP_PORT 80
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_POST "POST"
#define HTTP_RESPONSE_HEADER "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: application/json; charset=utf-8\nConnection: close\n\n"

extern CURRENT_STATE_T current_state;

typedef struct TCP_SERVER_T_ {
	struct tcp_pcb * server_pcb;
	bool complete;
	ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
	struct tcp_pcb * pcb;
	int sent_len;
	char method[8];
	char request[128];
	char params[128];
	char protocol[8];
	char headers[1024];
	char body[128];
	char result[128];
	int header_len;
	int result_len;
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
			free(con_state);
		}
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
	if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
		DEBUG_printf("[HTTP] [OK ] Send done\n");
		return tcp_close_client_connection(con_state, pcb, ERR_OK);
	}
	return ERR_OK;
}

static int handle_status_get (const char * request, const char * params, const char * body, char * result, size_t max_result_len) {
	return snprintf(result, max_result_len, "{volt: %f, temp: %f, power: %d}", current_state.voltage, current_state.tempC, current_state.power_state);
}

static int handle_power_post (const char * request, const char * params, const char * body, char * result, size_t max_result_len) {
	if (strstr(body, "requested_state")) {
		int requested_power_state_int;
		int led_param = sscanf(body, "requested_state=%d", &requested_power_state_int);
		if (led_param) {
			if  (requested_power_state_int == 0 || requested_power_state_int == 1) {
				bmc_power_handler((bool) requested_power_state_int);
				return snprintf(result, max_result_len, "{}");
			}
			else {
				return snprintf(result, max_result_len, "{error: true, description: \"invalid requested state, must be 0 or 1\"}"); 
			}
		}
		else {
			return snprintf(result, max_result_len, "{error: true, description: \"invalid requested state, must be 0 or 1\"}");
		}
	}
	else {
		return snprintf(result, max_result_len, "{error: true, description: \"missing required parameter requested_state\"}");
	}
}

int parse_content (struct pbuf * p, TCP_CONNECT_STATE_T * con_state) {
	char content[2048] = {0};
	pbuf_copy_partial(p, &content, 2048 - 1, 0);

	char * headers_start = content; // First header line containing method request?params HTTP/protocol
	char * headers_other = strstr(content, "\r\n") + 2; // remaining headers follow the first line
	char * body_start = strstr(content, "\r\n\r\n") + 4; // body begins at the end of headers

	*(headers_other - 2) = 0;
	*(body_start - 4) = 0;

	char request_params_comb[256];
	if (sscanf(headers_start, "%s %s HTTP/%s", con_state->method, request_params_comb, con_state->protocol) != 3) {
		return 0;
	}
  
	char * params = strchr(request_params_comb, '?');
	if (params) {
		*params++ = 0;
		strcpy(con_state->request, request_params_comb);
		strcpy(con_state->params, params);
	}
	else {
		strcpy(con_state->request, request_params_comb);
	}

	strcpy(con_state->headers, headers_other);
	strcpy(con_state->body, body_start);

	return 1;
}

int send_response (TCP_CONNECT_STATE_T * con_state, struct tcp_pcb * pcb) {
	// Check result buffer size
	if (con_state->result_len > sizeof(con_state->result) - 1) {
		DEBUG_printf("[HTTP] [ERR] Too much result data %d\n", con_state->result_len);
		return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
	}
	// Check header buffer size
	if (con_state->header_len > sizeof(con_state->headers) - 1) {
		DEBUG_printf("[HTTP] [ERR] Too much header data %d\n", con_state->header_len);
		return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
	}

	// Send the headers to the client
	con_state->sent_len = 0;
	err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
	if (err != ERR_OK) {
		DEBUG_printf("[HTTP] [ERR] Failed to write header data %d\n", err);
		return tcp_close_client_connection(con_state, pcb, err);
	}
	// Send the body to the client
	if (con_state->result_len) {
		err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
		if (err != ERR_OK) {
			DEBUG_printf("[HTTP] [ERR] Failed to write result data %d\n", err);
			return tcp_close_client_connection(con_state, pcb, err);
		}
	}
	return 0;
}

err_t tcp_server_recv(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err) {
	TCP_CONNECT_STATE_T * con_state = (TCP_CONNECT_STATE_T *)arg;
	if (!p) {
		DEBUG_printf("[HTTP] [ERR] Client connection closed\n");
		return tcp_close_client_connection(con_state, pcb, ERR_OK);
	}
	assert(con_state && con_state->pcb == pcb);
	if (p->tot_len > 0) {
		DEBUG_printf("[HTTP] [OK ] Server recieved %d err %d\n", p->tot_len, err);
		// Copy the request into the buffer

		if (!parse_content(p, con_state)) {
			DEBUG_printf("[HTTP] [ERR] Failed to parse header\n");
			return tcp_close_client_connection(con_state, pcb, ERR_OK); 
		}
		
		// print request
		DEBUG_printf("[HTTP] [OK ] Request: %s %s?%s %s\n", con_state->method, con_state->request, con_state->params, con_state->body);

		int response_code;

		// parse request depending on method and request
		if (strncmp(con_state->method, HTTP_GET, sizeof(HTTP_GET) - 1) == 0 && strncmp(con_state->request, "/status", sizeof("/status") - 1) == 0) {
			con_state->result_len = handle_status_get(con_state->request, con_state->params, con_state->body, con_state->result, sizeof(con_state->result));
			response_code = 200;
			con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADER, response_code, con_state->result_len);
		}
		else if (strncmp(con_state->method, HTTP_POST, sizeof(HTTP_POST) - 1) == 0 && strncmp(con_state->request, "/power", sizeof("/power") - 1) == 0) {
			con_state->result_len = handle_power_post(con_state->request, con_state->params, con_state->body, con_state->result, sizeof(con_state->result));
			response_code = 200;
			con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADER, response_code, con_state->result_len);
		}
		else {
			con_state->result_len = 0;
			response_code = 501;
			con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADER, response_code, con_state->result_len);
		}

		// print result
		DEBUG_printf("[HTTP] [OK ] Result: %d %s\n", response_code, con_state->result);

		if (send_response(con_state, pcb)) {
			DEBUG_printf("[HTTP] [ERR] Failure in send\n");
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
	tcp_server_close(http_serv_state);
	free(http_serv_state);
	http_serv_state = NULL;
	return 0;
}

#endif
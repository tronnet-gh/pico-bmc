#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "llhttp.h"

typedef struct HTTP_REQUEST_DATA_T_ {
	char protocol[8];
	char method[8];
	char url[128];
	char body[128];
} HTTP_REQUEST_DATA_T;

typedef struct HTTP_REQUEST_PARSER_WRAPPER_T_ {
	llhttp_t parser;
	llhttp_settings_t settings;
} HTTP_REQUEST_PARSER_WRAPPER_T;

int request_parser_on_url(llhttp_t * parser, const char * start, size_t length) {
	HTTP_REQUEST_DATA_T * http_request = parser->data;
	strncat(http_request->url, start, length);
	return 0;
}

int request_parser_on_body(llhttp_t * parser, const char * start, size_t length) {
	HTTP_REQUEST_DATA_T * http_request = parser->data;
	strncat(http_request->body, start, length);
	return 0;
}

int parse_http_request (HTTP_REQUEST_PARSER_WRAPPER_T * wrapper, char * content, size_t len) {
	llhttp_t parser = wrapper->parser;

	llhttp_errno_t err = llhttp_execute(&parser, content, len);
	HTTP_REQUEST_DATA_T * http_request = parser.data;

	uint8_t protocol_major = llhttp_get_http_major(&parser);
	uint8_t protocol_minor = llhttp_get_http_minor(&parser);
	sprintf(http_request->protocol, "%d.%d", protocol_major, protocol_minor);

	uint8_t method = llhttp_get_method(&parser);
	const char * method_name = llhttp_method_name(method);
	strcpy(http_request->method, method_name);

	return 0;
}

char * get_protocol (HTTP_REQUEST_PARSER_WRAPPER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->protocol;
}

char * get_method (HTTP_REQUEST_PARSER_WRAPPER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->method;
}

char * get_url (HTTP_REQUEST_PARSER_WRAPPER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->url;
}

char * get_body (HTTP_REQUEST_PARSER_WRAPPER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->body;
}

HTTP_REQUEST_PARSER_WRAPPER_T * new_request_parser () {
	HTTP_REQUEST_PARSER_WRAPPER_T * wrapper = calloc(1, sizeof(HTTP_REQUEST_PARSER_WRAPPER_T));

	// create a new request data struct
	HTTP_REQUEST_DATA_T * http_request = calloc(1, sizeof(HTTP_REQUEST_DATA_T));

	// init http parser settings
	llhttp_settings_init(&wrapper->settings);
	wrapper->settings.on_url = &request_parser_on_url;
	wrapper->settings.on_body = &request_parser_on_body;

	// init http request parser parser
	llhttp_init(&wrapper->parser, HTTP_REQUEST, &wrapper->settings);
	wrapper->parser.data = http_request;

	return wrapper;
}

void delete_request_parser (HTTP_REQUEST_PARSER_WRAPPER_T * wrapper) {
	free(wrapper->parser.data);
	free(wrapper);
}

int http_parser_init () {
	return 0;
}

int http_parser_deinit () {
	return 0;
}

#endif
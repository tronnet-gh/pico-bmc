#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "llhttp.h"

#define HTTP_RESPONSE_HEADER_TEMPLATE "HTTP/1.1 %d %s\nContent-Length: %d\nContent-Type: %s; charset=utf-8\nConnection: close\n\n"

typedef struct HTTP_REQUEST_DATA_T_ {
	llhttp_method_t method;
	char url[128];
	char body[128];
} HTTP_REQUEST_DATA_T;

typedef struct HTTP_REQUEST_PARSER_T_ {
	llhttp_t parser;
	llhttp_settings_t settings;
} HTTP_REQUEST_PARSER_T;

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

int parse_http_request (HTTP_REQUEST_PARSER_T * wrapper, char * content, size_t len) {
	llhttp_t parser = wrapper->parser;

	llhttp_errno_t err = llhttp_execute(&parser, content, len);
	HTTP_REQUEST_DATA_T * http_request = parser.data;

	llhttp_method_t method = llhttp_get_method(&parser);
	http_request->method = method;

	return 0;
}

llhttp_method_t get_method (HTTP_REQUEST_PARSER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->method;
}

char * get_url (HTTP_REQUEST_PARSER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->url;
}

char * get_body (HTTP_REQUEST_PARSER_T * wrapper) {
	HTTP_REQUEST_DATA_T * http_request = wrapper->parser.data;
	return http_request->body;
}

HTTP_REQUEST_PARSER_T * new_request_parser () {
	// create new response parser wrapper
	HTTP_REQUEST_PARSER_T * wrapper = calloc(1, sizeof(HTTP_REQUEST_PARSER_T));

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

void delete_request_parser (HTTP_REQUEST_PARSER_T * wrapper) {
	free(wrapper->parser.data);
	free(wrapper);
}

typedef struct HTTP_RESPONSE_DATA_T_ {
	llhttp_status_t status;
	char type[128];
	char body[128];
} HTTP_RESPONSE_DATA_T;

typedef struct HTTP_RESPONSE_COMPOSER_T_ {
	HTTP_RESPONSE_DATA_T * response_data;
	char header[1024];
	char body[128];
	uint32_t header_length;
	uint32_t content_length;
} HTTP_RESPONSE_COMPOSER_T;

void set_status (HTTP_RESPONSE_COMPOSER_T * composer, llhttp_status_t status) {
	HTTP_RESPONSE_DATA_T * response_data = composer->response_data;
	response_data->status = status;
}

void set_type (HTTP_RESPONSE_COMPOSER_T * composer, char * type) {
	HTTP_RESPONSE_DATA_T * response_data = composer->response_data;
	strncpy(response_data->type, type, 128);
}

void set_body (HTTP_RESPONSE_COMPOSER_T * composer, char * body) {
	HTTP_RESPONSE_DATA_T * response_data = composer->response_data;
	strncpy(response_data->body, body, 128);
}

void compose_http_response (HTTP_RESPONSE_COMPOSER_T * composer) {
	HTTP_RESPONSE_DATA_T * response_data = composer->response_data;

	composer->content_length = snprintf(composer->body, sizeof(composer->body), "%s", response_data->body);

	const char * status_name = llhttp_status_name(response_data->status);

	composer->header_length = snprintf(composer->header, sizeof(composer->header), HTTP_RESPONSE_HEADER_TEMPLATE, response_data->status, status_name, composer->content_length, response_data->type);
}

HTTP_RESPONSE_COMPOSER_T * new_response_composer () {
	// create new response composer wrapper
	HTTP_RESPONSE_COMPOSER_T * wrapper = calloc(1, sizeof(HTTP_RESPONSE_COMPOSER_T));
	
	// create a new response data struct
	HTTP_RESPONSE_DATA_T * http_response = calloc(1, sizeof(HTTP_RESPONSE_DATA_T));

	wrapper->response_data = http_response;

	return wrapper;
}

void delete_response_composer (HTTP_RESPONSE_COMPOSER_T * composer) {
	free(composer->response_data);
	free(composer);
}

int http_parser_init () {
	return 0;
}

int http_parser_deinit () {
	return 0;
}

#endif
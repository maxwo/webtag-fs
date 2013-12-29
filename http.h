#include <errno.h>
#include <string.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#ifndef __HTTP_H__
#define __HTTP_H__

const unsigned short DATA_TYPE_MEMORY = 1;
const unsigned short DATA_TYPE_FILE = 2;
const unsigned short DATA_TYPE_JSON = 3;

const unsigned short REQUEST_STATUS_INITIALIZED = 1;
const unsigned short REQUEST_STATUS_PROCESSING = 2;
const unsigned short REQUEST_STATUS_FINISHED = 3;
const unsigned short REQUEST_STATUS_ERROR = 4;

const char *TMP_FILE_TEMPLATE = "/tmp/http.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

struct Response {
    unsigned short http_status;
    char *url;
    unsigned short data_type;
    char *data;
    size_t size;
};

struct Request {
	unsigned short status;
	char *url;
	unsigned short data_type;
    struct Response *response;
};

struct Cache {
    struct Response *response;
    struct Cache *next;
};

struct Connection {
    char *domain;
    struct Cache *cache;
};

struct WriteCallbackStruct {
	struct Request *request;
	struct Response *response;
	FILE *file;
};


void
	init_request(struct Connection *connection, struct Request *request, const char *uri, const unsigned short data_type);

void
	destroy_request(struct Request *request);

void
	init_response(struct Response *response, struct Request *request);
	
size_t
	read_response(struct Response *response, char *buf, size_t size, off_t offset);

void
	destroy_response(struct Response *response);

size_t
	write_callback(void *data, size_t size, size_t nmemb, void *user);

int
	check_cache(struct Connection *connection, struct Request *request);

void
	save_cache(struct Connection *connection, struct Response *response);

int
	process_request(struct Connection *connection, struct Request *request);

void
	init_connection(struct Connection *connection, const char *domain);

void
	destroy_connection(struct Connection *connection);

#endif
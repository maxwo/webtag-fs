#include "http.h"

void
create_url(struct Connection *connection, char *destination, const char *uri) {
    char *end = stpcpy(destination, connection->domain);
    strcpy(end, uri);
}

void
init_request(struct Connection *connection, struct Request *request, const char *uri, const unsigned short data_type) {
	
	printf("INIT REQUEST\n");
	
    size_t url_len = strlen(connection->domain);
    url_len += strlen(uri);
    request->url = malloc(url_len+1);
	//syslog (LOG_INFO, "Request initialization %us for %s", data_type, uri);
    create_url(connection, request->url, uri);
    
    request->data_type = data_type;
	request->status = REQUEST_STATUS_INITIALIZED;
}

void
destroy_request(struct Request *request) {	
	
	printf("DESTROY REQUEST\n");
	
    free(request->url);
	
	// Not cached
	if ( request->status==REQUEST_STATUS_ERROR ) {
		destroy_response(request->response);
		free(request->response);
	}
}

void
init_response(struct Response *response, struct Request *request) {	
	
	printf("INIT RESPONSE\n");
	
	int url_len = strlen(request->url);
	response->url = malloc(sizeof(char) * url_len);
	strcpy(response->url, request->url);
	response->http_status = 0;
	response->data_type = request->data_type;
	response->size = 0;
}

void
destroy_response(struct Response *response) {
		
	printf("DESTROY RESPONSE\n");
	
	free(response->url);
	free(response->data);
	//TODO remove file
}

size_t
write_callback(void *data, size_t size, size_t nmemb, void *user) {	
	printf("WRITE CALLBACK\n");
    size_t realsize = size * nmemb;
    struct WriteCallbackStruct *wcs = (struct WriteCallbackStruct *)user;
	
	if ( wcs->request->data_type==DATA_TYPE_MEMORY ) {
		
	    wcs->response->data = realloc(wcs->response->data, wcs->response->size+realsize);
	    if (wcs->response->data == NULL) {
	        return -1;
	    }
	    memcpy(&(wcs->response->data[wcs->response->size]), data, realsize);
	    wcs->response->size += realsize;
		return realsize;
	
	} else if ( wcs->request->data_type==DATA_TYPE_FILE ) {
		
	    wcs->response->size += realsize;
		return fwrite(data, size, nmemb, wcs->file);
		
	}
	
    return -1;
}

int
check_cache(struct Connection *connection, struct Request *request) {
	
	printf("CHECK CACHE\n");
	
	struct Cache *current = connection->cache;
	while ( current!=0 ) {
		if ( !strcmp(request->url, current->response->url) ) {
			request->response = current->response;
			return 1;
		}
		current = current->next;
	}
	return 0;
}

void
save_cache(struct Connection *connection, struct Response *response) {
	
	printf("SAVE CACHE\n");

	struct Cache *cache;
	cache = malloc(sizeof(struct Cache));
	cache->response = response;
	cache->next = connection->cache;
	connection->cache = cache;
}

int
process_request(struct Connection *connection, struct Request *request) {
	
	printf("PROCESS REQUEST\n");

    CURL *curl_handle;
    CURLcode res;
	struct WriteCallbackStruct wcs;
	struct Response *response;
	
	if ( check_cache(connection, request) ) {
		printf("USING CACHE\n");
		request->status = REQUEST_STATUS_FINISHED;
		return request->response->http_status;
	}
	
	response = malloc(sizeof(struct Response));
    memset(response, 0, sizeof(struct Response));
	
	init_response(response, request);
	
	request->status = REQUEST_STATUS_PROCESSING;
	
	printf("INITIALIZING DATA\n");
	
    if ( request->data_type==DATA_TYPE_FILE ) {
        response->data = malloc(strlen(TMP_FILE_TEMPLATE));
        strcpy(response->data, TMP_FILE_TEMPLATE);
        mkstemp(response->data);
	    wcs.file = fopen (response->data,"w");
    } else if ( request->data_type==DATA_TYPE_MEMORY ) {
        response->data = malloc(1);
    }

	wcs.request = request;
	wcs.response = response;
	
	printf("LAUNCHING REQUEST\n");

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, request->url);
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &wcs);

    res = curl_easy_perform(curl_handle);
    
    if ( request->data_type==DATA_TYPE_FILE ) {
        fclose(wcs.file);
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

	response->http_status = res;
	request->response = response;
	
	if ( res==CURLE_OK ) {
		request->status = REQUEST_STATUS_FINISHED;
		save_cache(connection, response);
	} else {	
		request->status = REQUEST_STATUS_ERROR;
	}

    return res!=CURLE_OK;
}

void
init_connection(struct Connection *connection, const char *domain) {
	size_t domain_len = strlen(domain);
	connection->domain = malloc(sizeof(char) * domain_len);
	connection->cache = 0;
	strcpy(connection->domain, domain);
}

void
destroy_connection(struct Connection *connection) {
	struct Cache *current = connection->cache;
	struct Cache *next = 0;
	while ( current ) {
		next = current->next;
		destroy_response(current->response);
		free(current);
		current = next;
	}
	free(connection->domain);
}




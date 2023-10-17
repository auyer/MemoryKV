#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// string_carrier struct to store the body results from curl with easy reallocation
typedef struct string_carrier {
	char *ptr;
	size_t len;
	bool error_flag;
} string_carrier;

// init_string_carrier initializes the string_carrier struct to store the body results from curl
void init_string_carrier(string_carrier *s) {
	if (s == NULL) {
		printf("Error in init_string_carrier");
		return;
	}
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	s->error_flag = false;
	if (s->ptr == NULL) {
		// TODO: figure out a better way to handle errors in C
		fprintf(stderr, "memkv client: Error in INIT, malloc() failed\n");
		// skip exit and return error instead
		// exit(EXIT_FAILURE);
		s->error_flag = true;
	} else {
		s->ptr[0] = '\0';
	}
}

string_carrier *new_string_carrier() {
	string_carrier *s = malloc(sizeof *s);
	init_string_carrier(s);
	if (s == NULL || s->error_flag || s->ptr == NULL) {
		printf("Error in new_string_carrier\n");
		return NULL;
	}
	return s;
}

// string_carrier_writefunc is the callback function to read the body from libcurl into a string_carrier
size_t string_carrier_writefunc(void *ptr, size_t size, size_t nmemb, struct string_carrier *s) {
	// new length to realocate response chunks from libcurl
	size_t new_len = s->len + size * nmemb;
	// s->ptr = realloc(s->ptr, new_len + 1);
	void *const tmp = realloc(s->ptr, new_len + 1);
	if (!tmp) {
		// TODO: figure out a better way to handle errors in C
		fprintf(stderr, "memkv client: Error in string_carrier_writefunc Callback, realloc() failed\n");
		// skip exit and return error instead
		// exit(EXIT_FAILURE);
		s->error_flag = true;

		/* realloc() failed to reallocate memory. The original memory is still
		 * there and needs to be freed.
		 */
		// handle_failure();
	} else {
		/* Now s->ptr points to the new chunk of memory. */
		s->ptr = tmp;
		memcpy(s->ptr + s->len, ptr, size * nmemb);
		s->len = new_len;
	}

	return size * nmemb;
}

// memkv_client
typedef struct memkv_client {
	char *host;
} memkv_client;

void memkv_init_client(memkv_client *client, char *host) {
	curl_global_init(CURL_GLOBAL_ALL);
	client->host = host;
}

memkv_client *memkv_client_new(char *host) {
	memkv_client *client = malloc(sizeof *client);
	if (client == NULL) {
		printf("Error in memkv_client_new");
		return NULL;
	}
	memkv_init_client(client, host);
	return client;
}

char *build_url(const char *host, const char *params) {
	// snprintf returns the number of characters that would have been written if called with NULL
	unsigned long size_needed = snprintf(NULL, 0, "%s/%s", host, params);
	// use that number to allocate a buffer of the right size
	char *url = malloc(size_needed + 1);
	// write the string_carrier to the buffer
	sprintf(url, "%s/%s", host, params);
	return url;
}

typedef struct {
	bool success;
	union {
		char *result;
		char *error;
	};
} memkv_result;

static const char base_curl_error[] = "Error from curl";
static const char unknown_error_msg[] = "unknown error";

void make_curl_error(memkv_result *r, const char *err) {
	r->success = false;

	if (strlen(err) == 0) {
		r->error = malloc(sizeof(unknown_error_msg));
		// strlcpy(r->error, unknown_error_msg, sizeof(unknown_error_msg) + sizeof(r->error));
		strcpy(r->error, unknown_error_msg);

		return;
	}

	unsigned long size_needed = snprintf(NULL, 0, "%s : %s", base_curl_error, err);

	r->error = malloc(size_needed + 1);

	snprintf(r->error, size_needed + 1, "%s : %s", base_curl_error, err);
}

// init_memkv_result initializes the memkv_result struct with success as false
memkv_result *init_memkv_result() {

	memkv_result *r = malloc(sizeof *r);

	r->result = malloc(1);
	if (r->result == NULL) {
		free(r);
		printf("Error in init_memkv_result");
		return NULL;
	}

	r->success = false;
	return r;
}

memkv_result *memkv_execute_request(
	const char *url,
	const char *custom_req,
	const char *content_type,
	const char *post_data) {

	memkv_result *r = init_memkv_result();

	CURLcode res;

	// TODO move this to the client ?
	CURL *curl = curl_easy_init();

	if (!curl) {
		make_curl_error(r, "Failed starting curl.");
		return r;
	}

	string_carrier *s = new_string_carrier();

	if (s == NULL || s->error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	int curl_parameter_responses = 0;
	curl_parameter_responses += curl_easy_setopt(curl, CURLOPT_URL, url);

	if (custom_req) {
		curl_parameter_responses += curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, custom_req);
	}

	struct curl_slist *headers = NULL;

	if (content_type) {
		/* default type with postfields is application/x-www-form-urlencoded,
		   change it if you want. */
		headers = curl_slist_append(headers, content_type);
		curl_parameter_responses += curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}

	if (post_data) {
		curl_parameter_responses += curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	}

	curl_parameter_responses += curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	printf("Curl parameters responses %d %s\n", curl_parameter_responses, curl_parameter_responses ? "true" : "false");

	int curl_body_responses = 0;

	curl_body_responses += curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_body_responses += curl_easy_setopt(curl, CURLOPT_WRITEDATA, s);

	printf("Curl body responses %d %s\n", curl_body_responses, curl_body_responses ? "true" : "false");

	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

	if (headers) {
		curl_slist_free_all(headers);
	}

	if (res != CURLE_OK) {
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s->error_flag) {
		make_curl_error(r, "Failed to get results from server.");
		return r;
	}

	r->success = true;
	r->result = malloc(strlen(s->ptr) + 1);

	strcpy(r->result, s->ptr);
	// strlcpy(r->result, s->ptr, sizeof(r->result) + sizeof(s->ptr));
	return r;
}

memkv_result *memkv_get_key(memkv_client *client, const char *key) {
	char *url = build_url(client->host, key);
	memkv_result *r = memkv_execute_request(url, NULL, NULL, NULL);
	free(url);
	return r;
}

memkv_result *memkv_delete_key(memkv_client *client, const char *key) {
	char *url = build_url(client->host, key);
	memkv_result *r = memkv_execute_request(url, "DELETE", NULL, NULL);
	free(url);
	return r;
}

memkv_result *memkv_put_key(memkv_client *client, const char *key, const char *put_body) {
	char *url = build_url(client->host, key);
	memkv_result *r = memkv_execute_request(url, "PUT", "Content-Type: application/octet-stream", put_body);
	free(url);
	return r;
}

memkv_result *memkv_list_keys(memkv_client *client) {
	char *url = build_url(client->host, "keys");
	memkv_result *r = memkv_execute_request(url, NULL, NULL, NULL);
	free(url);
	return r;
}

memkv_result *memkv_list_keys_with_prefix(memkv_client *client, const char *key_prefix) {
	char *url_part1 = build_url(client->host, "keys");
	// TODO: build URL in one call
	char *url = build_url(url_part1, key_prefix);
	free(url_part1);

	memkv_result *r = memkv_execute_request(url, NULL, NULL, NULL);
	free(url);
	return r;
}

memkv_result *memkv_delete_keys_with_prefix(memkv_client *client, const char *key_prefix) {
	char *url_part1 = build_url(client->host, "keys");
	// TODO: build URL in one call
	char *url = build_url(url_part1, key_prefix);
	free(url_part1);
	memkv_result *r = memkv_execute_request(url, "DELETE", NULL, NULL);
	free(url);
	return r;
}

memkv_result *memkv_delete_all_keys(memkv_client *client) {
	char *url = build_url(client->host, "keys");
	memkv_result *r = memkv_execute_request(url, "DELETE", NULL, NULL);
	free(url);
	return r;
}

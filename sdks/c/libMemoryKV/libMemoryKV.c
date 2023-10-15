#include <curl/curl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// string_carrier struct to store the body results from curl with easy reallocation
typedef struct string_carrier {
	char *ptr;
	size_t len;
	bool error_flag;
} string_carrier;

// init_string_carrier initializes the string_carrier struct to store the body results from curl
void init_string_carrier(struct string_carrier *s) {
	if (s->ptr == NULL) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    s->error_flag = false;
	} else {
    memset(s, 0, sizeof *s);
  }
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

void memkv_init_client(struct memkv_client *client, char *host) {
	curl_global_init(CURL_GLOBAL_ALL);
	client->host = host;
}

memkv_client *memkv_client_new(char *host) {
	memkv_client *client = malloc(sizeof(memkv_client));
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
		strcpy(r->error, unknown_error_msg);
		return;
	}

	unsigned long size_needed = snprintf(NULL, 0, "%s : %s", base_curl_error, err);

	r->error = malloc(size_needed + 1);

	snprintf(r->error, size_needed + 1, "%s : %s", base_curl_error, err);
}

// init_memkv_result initializes the memkv_result struct with success as false
memkv_result *init_memkv_result() {

	memkv_result *r = malloc(sizeof(memkv_result));

	r->result = malloc(sizeof(char));
	if (r->result == NULL) {
		free(r);
		printf("Error in init_memkv_result");
		return NULL;
	}

	r->success = false;
	return r;
}

memkv_result *memkv_get_key(struct memkv_client *client, const char *key) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}

	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	char *url = build_url(client->host, key);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	if (res != CURLE_OK) {
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}
	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

memkv_result *memkv_delete_key(struct memkv_client *client, const char *key) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}

	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	char *url = build_url(client->host, key);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	// set custom request to delete
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	/* Check for errors */
	if (res != CURLE_OK) {
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

memkv_result *memkv_put_key(struct memkv_client *client, const char *key, const char *put_body) {
	memkv_result *r = init_memkv_result();
	CURL *curl;
	CURLcode res;
	// start the easy interface in curl
	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}
	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	char *url = build_url(client->host, key);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	struct curl_slist *headers = NULL;

	/* default type with postfields is application/x-www-form-urlencoded,
	   change it if you want */
	headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	/* pass on content in request body. When CURLOPT_POSTFIELDSIZE is not used,
	curl does strlen to get the size. */
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, put_body);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);
	/* free headers */
	curl_slist_free_all(headers);

	/* Check for errors */
	if (res != CURLE_OK) {
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

memkv_result *memkv_list_keys(struct memkv_client *client) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	// start the easy interface in curl
	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}
	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	const char *key = "keys";
	char *url = build_url(client->host, key);

	// set custom request to delete
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

memkv_result *memkv_list_keys_with_prefix(struct memkv_client *client, const char *key_prefix) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	// start the easy interface in curl
	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}
	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	const char *key = "keys";
	char *url_part1 = build_url(client->host, key);
	// TODO: build URL in one call
	char *url = build_url(url_part1, key_prefix);
	free(url_part1);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

memkv_result *memkv_delete_keys_with_prefix(struct memkv_client *client, const char *key_prefix) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}

	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	const char *key = "keys";
	char *url_part1 = build_url(client->host, key);
	// TODO: build URL in one call
	char *url = build_url(url_part1, key_prefix);
	free(url_part1);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	// set custom request to delete
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	/* Check for errors */
	if (res != CURLE_OK) {
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

memkv_result *memkv_delete_all_keys(struct memkv_client *client) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	// start the easy interface in curl
	curl = curl_easy_init();
	if (!curl) {
		make_curl_error(r, "Failed startig curl");
		return r;
	}
	struct string_carrier s;
	init_string_carrier(&s);
	if (s.error_flag) {
		make_curl_error(r, "Failed to initialize string_carrier");
		return r;
	}

	const char *key = "keys";
	char *url = build_url(client->host, key);

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);

	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		make_curl_error(r, err);
		return r;
	}

	if (s.error_flag) {
		make_curl_error(r, "Failed to get results from server");
		return r;
	}
	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}

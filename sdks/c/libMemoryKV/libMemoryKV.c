#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// string struct to store the body results from curl with easy reallocation
struct string {
	char *ptr;
	size_t len;
};

// init_string initializes the string struct to store the body results from curl
void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		// TODO: figure out a better way to handle errors in C
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

// writefunc is the callback function to read the body from libcurl into a string
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
	// new length to realocate response chunks from libcurl
	size_t new_len = s->len + size * nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		// TODO: figure out a better way to handle errors in C
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size * nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size * nmemb;
}

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
	unsigned long sizeneeded = snprintf(NULL, 0, "%s/%s", host, params);
	// use that number to allocate a buffer of the right size
	char *url = malloc(sizeneeded + 1);
	// write the string to the buffer
	sprintf(url, "%s/%s", host, params);
	return url;
}

typedef struct {
	bool success : 1;
	union {
		char *result;
		char *error;
	};
} memkv_result;

// Request functions:
//

memkv_result *make_curl_error(memkv_result *r, char *err) {
	r->success = false;
	// create and alloc error
	unsigned long sizeneeded = snprintf(NULL, 0, "%s :%s", "Error from curl", err);
	r->error = malloc(sizeneeded + 1);
	sprintf(r->error, "%s :%s", "Error from curl", err);
	return r;
}

memkv_result *init_memkv_result() {

	memkv_result *r = malloc(sizeof(memkv_result));
	memkv_result rinit = {.success = false, .error = ""};
	r = &rinit;
	return r;
}

memkv_result *memkv_get_key(struct memkv_client *client, const char *key) {
	memkv_result *r = init_memkv_result();

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		return make_curl_error(r, "Failed startig curl");
	}

	struct string s;
	init_string(&s);

	char *url = build_url(client->host, key);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* example.com is redirected, so we tell libcurl to follow redirection */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		return make_curl_error(r, err);
	}
	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

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
		return make_curl_error(r, "Failed startig curl");
	}

	struct string s;
	init_string(&s);

	char *url = build_url(client->host, key);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* example.com is redirected, so we tell libcurl to follow redirection */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	// set custom request to delete
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		return make_curl_error(r, err);
	}
	free(url);
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
		return make_curl_error(r, "Failed startig curl");
	}
	struct string s;
	init_string(&s);

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

	/* override the POST implied by CURLOPT_POSTFIELDS
	 *
	 * Warning: CURLOPT_CUSTOMREQUEST is problematic, especially if you want
	 * to follow redirects. Be aware.
	 */

	/* example.com is redirected, so we tell libcurl to follow redirection */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		return make_curl_error(r, err);
	}
	/* always cleanup */
	curl_easy_cleanup(curl);

	/* free headers */
	curl_slist_free_all(headers);
	free(url);
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
		return make_curl_error(r, "Failed startig curl");
	}
	struct string s;
	init_string(&s);

	const char *key = "keys";
	char *url = build_url(client->host, key);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* example.com is redirected, so we tell libcurl to follow redirection */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
		r->success = false;
		const char *err = curl_easy_strerror(res);
		return make_curl_error(r, err);
	}
	/* always cleanup */
	curl_easy_cleanup(curl);
	free(url);

	r->success = true;
	r->result = malloc(strlen(s.ptr) + 1);

	strcpy(r->result, s.ptr);
	return r;
}
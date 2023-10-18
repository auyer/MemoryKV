#include <curl/curl.h>
#include <math.h>
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
static void init_string_carrier(string_carrier *s) {
	s->len = 0;
	s->error_flag = false;
	s->ptr = malloc(s->len + 1);
	if (s->ptr != NULL) {
		// set ptr as null terminator
		s->ptr[0] = '\0';
	} else {
		// set error flag to true if malloc fails
		s->error_flag = true;
		fprintf(stderr, "memkv client: Error in INIT, malloc() failed\n");
	}
}

static string_carrier *new_string_carrier() {
	string_carrier *s = malloc(sizeof *s);
	if (!s) {
		free(s);
		return NULL;
	}
	init_string_carrier(s);
	if (s->error_flag || s->ptr == NULL) {
		free(s);
		printf("Error in new_string_carrier\n");
		return NULL;
	}
	return s;
}

// string_carrier_writefunc is the callback function to read the body from libcurl into a string_carrier
static size_t string_carrier_writefunc(void *ptr, size_t size, size_t nmemb, struct string_carrier *s) {
	// new length to realocate response chunks from libcurl
	size_t new_len = s->len + (size * nmemb);
	void *const tmp = realloc(s->ptr, new_len + 1);
	if (!tmp) {
		fprintf(stderr, "memkv client: Error in string_carrier_writefunc Callback, realloc() failed\n");
		s->error_flag = true;
	} else {
		/* Now s->ptr points to the new chunk of memory. */
		s->ptr = tmp;
		// we already know the length of the string, so we can just copy it with memcpy instead of strlcpy
		memcpy(s->ptr + s->len, ptr, new_len + 1);
		s->len = new_len;
	}

	return size * nmemb;
}

// memkv_client stores the configurations used to connect to the MemoryKV server
// use `memkv_client_new` to create one, or `memkv_init_client` to initialize an existing instance
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

static char *build_url(const char *host, const char *params) {
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

// write_result_error sets result as Error with a message
static void write_result_error(memkv_result *r, const char *err) {
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

// curl errors are ints smaller than 100. This value was chosen as an offset to store all errors in a single int
static const int ERR_OFFSET = 100;

// accumulate_curl_errors takes an int and multiplies it by ERR_OFFSET to store the previous error and the new one in a
// single int
static int accumulate_curl_errors(int err_accumulator, int new_error) {
	if (new_error == 0) {
		return err_accumulator;
	}
	if (err_accumulator == 0) {
		return new_error;
	}
	return (err_accumulator * ERR_OFFSET) + new_error;
}

// err_accumulator_to_string makes a string out of a single integer that contains multiple errors by dividing it by
// ERR_OFFSET
static char *err_accumulator_to_string(int err_accumulator) {
	// log(err_accumulator) on base ERR_OFFSET gives the number of errors
	int len = log10(err_accumulator) / log10(ERR_OFFSET);
	// to store in a string array
	int str_len;
	if (len == 1) {
		// log ERR_OFFSET is the number of characters in the error message
		// +1 for the null terminator at the end
		str_len = log10(ERR_OFFSET) + 1;
	} else {
		// log ERR_OFFSET is the number of characters in the error message
		// +1 for the comma and the null terminator at the end (since there will be no comma)
		// times the number of errors
		str_len = (log10(ERR_OFFSET) + 1) * len;
	}
	char *err_array = malloc(str_len);
	for (int i = 0; i < len; i++) {
		snprintf(err_array, str_len, "%s,%d", err_array, (err_accumulator % ERR_OFFSET));
		err_accumulator = err_accumulator / ERR_OFFSET;
	}

	return err_array;
}

static memkv_result *memkv_execute_request(
	const char *url,
	const char *custom_req,
	const char *content_type,
	const char *post_data) {

	memkv_result *r = init_memkv_result();

	CURLcode res;

	// TODO move this to the client ?
	CURL *curl = curl_easy_init();

	if (!curl) {
		write_result_error(r, "Failed starting curl.");
		return r;
	}

	string_carrier *s = new_string_carrier();

	if (s == NULL || s->error_flag) {
		write_result_error(r, "Failed to initialize string_carrier");
		return r;
	}

	int curl_parameter_errors = 0;
	accumulate_curl_errors(curl_parameter_errors, curl_easy_setopt(curl, CURLOPT_URL, url));

	if (custom_req) {
		accumulate_curl_errors(curl_parameter_errors, curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, custom_req));
	}

	struct curl_slist *headers = NULL;

	if (content_type) {
		/* default type with postfields is application/x-www-form-urlencoded,
		   change it if you want. */
		headers = curl_slist_append(headers, content_type);
		accumulate_curl_errors(curl_parameter_errors, curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
	}

	if (post_data) {
		accumulate_curl_errors(curl_parameter_errors, curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data));
	}

	accumulate_curl_errors(curl_parameter_errors, curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L));
	if (curl_parameter_errors != 0) {
		printf("Curl returned errors while setting parameters: %s\n", err_accumulator_to_string(curl_parameter_errors));
		write_result_error(r, err_accumulator_to_string(curl_parameter_errors));
		return r;
	}

	int curl_body_errors = 0;

	accumulate_curl_errors(curl_body_errors, curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_carrier_writefunc));
	accumulate_curl_errors(curl_body_errors, curl_easy_setopt(curl, CURLOPT_WRITEDATA, s));

	if (curl_body_errors != 0) {
		printf("Curl returned errors while setting body: %s\n", err_accumulator_to_string(curl_body_errors));
		write_result_error(r, err_accumulator_to_string(curl_parameter_errors));
		return r;
	}

	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

	if (headers) {
		curl_slist_free_all(headers);
	}

	if (res != CURLE_OK) {
		const char *err = curl_easy_strerror(res);
		write_result_error(r, err);
		return r;
	}

	if (s->error_flag) {
		write_result_error(r, "Failed to get results from server.");
		return r;
	}

	r->success = true;
	// updates the result pointer to point to the string_carrier->ptr
	r->result = s->ptr;
	free(s);

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

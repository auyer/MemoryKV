#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	int sizeneeded = snprintf(NULL, 0, "%s/%s", host, params);
	// use that number to allocate a buffer of the right size
	char *url = malloc(sizeneeded + 1);
	// write the string to the buffer
	sprintf(url, "%s/%s", host, params);
	return url;
}

char memkv_get_key(struct memkv_client *client, const char *key) {

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		char *url = build_url(client->host, key);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
	} else {
		return 0;
	}
}

char memkv_delete_key(struct memkv_client *client, const char *key) {

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (curl) {

		char *url = build_url(client->host, key);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		// set custom request to delete
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
	} else {
		return 0;
	}
}

char memkv_put_key(struct memkv_client *client, const char *key, const char *put_body) {
	CURL *curl;
	CURLcode res;
	// start the easy interface in curl
	curl = curl_easy_init();
	if (curl) {
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
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* free headers */
		curl_slist_free_all(headers);
	} else {
		return 0;
	}
}

char memkv_list_keys(struct memkv_client *client) {
	CURL *curl;
	CURLcode res;

	// start the easy interface in curl
	curl = curl_easy_init();
	if (curl) {

		char *key = "keys";
		char *url = build_url(client->host, key);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
		free(url);
	} else {
		return 0;
	}
}
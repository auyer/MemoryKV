#include <curl/curl.h>
#include <stdio.h>

char get_key(char *key) {

  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {

    char base_url[] = "http://localhost:8080/";

    char url[sizeof(key) + sizeof(base_url)];

    snprintf(url, sizeof(url), "http://localhost:8080/%s", key);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  } else {
    return 0;
  }
}

char delete_key(char *key) {

  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char base_url[] = "http://localhost:8080/";

    char url[sizeof(key) + sizeof(base_url)];

    snprintf(url, sizeof(url), "http://localhost:8080/%s", key);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // set custom request to delete
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  } else {
    return 0;
  }
}

char put_key(char *key, char *put_body) {
  CURL *curl;
  CURLcode res;
  // start the easy interface in curl
  curl = curl_easy_init();
  if (curl) {
    char base_url[] = "http://localhost:8080/";

    char url[sizeof(key) + sizeof(base_url)];

    snprintf(url, sizeof(url), "http://localhost:8080/%s", key);

    struct curl_slist *headers = NULL;

    /* default type with postfields is application/x-www-form-urlencoded,
       change it if you want */
    headers =
        curl_slist_append(headers, "Content-Type: application/octet-stream");
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

    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);

    /* free headers */
    curl_slist_free_all(headers);
  } else {
    return 0;
  }
}

char list_keys() {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);

  // start the easy interface in curl
  curl = curl_easy_init();
  if (curl) {

    char base_url[] = "http://localhost:8080/";
    char key[] = "keys";

    char url[sizeof(key) + sizeof(base_url)];

    snprintf(url, sizeof(url), "http://localhost:8080/%s", key);

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/keys");
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  } else {
    return 0;
  }
}
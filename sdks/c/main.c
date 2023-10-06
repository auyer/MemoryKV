#include <curl/curl.h>
#include <libMemoryKV.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	memkv_client *client;

	client = memkv_client_new("http://localhost:8080");

	char key[] = "c_sdk2";

	// put key
	fprintf(stdout, "\nPut Key Request\n");

	static const char put_body[] = "{"
								   " \"name\" : \"c_sdk\","
								   " \"content\" : \"json\""
								   "}";

	char *response;
	response = memkv_put_key(client, key, put_body);
	printf("%s\n", response);
	free(response);

	fprintf(stdout, "\nList Keys Request\n");
	// list key
	response = memkv_list_keys(client);
	printf("%s\n", response);
	free(response);

	// get key
	fprintf(stdout, "\nMaking a Get Key Request\n");

	response = memkv_get_key(client, key);
	printf("%s\n", response);
	free(response);

	// delete key
	fprintf(stdout, "\nMaking a delete Key Request\n");

	response = memkv_delete_key(client, key);
	printf("%s\n", response);
	free(response);

	free(client);
}

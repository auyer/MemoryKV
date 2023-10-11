#include <curl/curl.h>
#include <libMemoryKV.h>
#include <stdio.h>
#include <stdlib.h>

void print_memkv_result(memkv_result *response) {
	if (response->success) {
		printf("Success: %s\n", response->result);
	} else {
		fprintf(stderr, "Error: %s\n", response->error);
	}
}

int main(void) {
	memkv_client *client;

	client = memkv_client_new("http://localhost:8080");

	memkv_result *response = malloc(sizeof(memkv_result));
	
  char key[] = "c_sdk";
  char key2[] = "c_sdk2";

	// put key
	static const char put_body[] = "{"
								   " \"name\" : \"c_sdk\","
								   " \"content\" : \"json\""
								   "}";


	fprintf(stdout, "\nPut Key Request\n");
  response = memkv_put_key(client, key, put_body);

	print_memkv_result(response);

	fprintf(stdout, "\nList Keys Request\n");
	// list key
	response = memkv_list_keys(client);
	print_memkv_result(response);

	// get key
	fprintf(stdout, "\nMaking a Get Key Request\n");

	response = memkv_get_key(client, key);
	print_memkv_result(response);

  fprintf(stdout, "\nPut Key Request 2\n");
  response = memkv_put_key(client, key2, put_body);

	print_memkv_result(response);

	// list key
  fprintf(stdout, "\nList Keys Request 2\n");
	response = memkv_list_keys(client);
	print_memkv_result(response);

	// delete key
	fprintf(stdout, "\nMaking a delete Key Request\n");

	response = memkv_delete_key(client, key);
	print_memkv_result(response);

	fprintf(stdout, "\nMaking a delete Key Request2\n");

	response = memkv_delete_key(client, key2);
	print_memkv_result(response);

	free(client);
}

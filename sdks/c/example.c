#include <curl/curl.h>
#include <libMemoryKV.h>
#include <stdio.h>
#include <stdlib.h>

void print_memkv_result(memkv_result *response) {
	if (!response) {
		fprintf(stderr, "Error: response is NULL\n");
		return;
	}
	if (response->success) {
		printf("Success! Result: %s\n", response->result);
	} else {
		fprintf(stderr, "Error: %s\n", response->error);
	}
}

int main(void) {
	memkv_client *client;

	client = memkv_client_new("http://localhost:8080");

	memkv_result *response;

	char key[] = "c_sdk";
	char key2[] = "c_sdk2";
	char key3[] = "a_sdk";

	fprintf(stdout, "This example will create, and delete in 3 keys: %s, %s and %s\n", key, key2, key3);

	// put key
	static const char put_body[] = "{"
								   " \"name\" : \"MemoryKV Example Body\","
								   " \"content\" : \"json\""
								   "}";

	fprintf(stdout, "\nPut Key '%s'\n", key);
	response = memkv_put_key(client, key, put_body);
	print_memkv_result(response);
	free(response);

	// list key
	fprintf(stdout, "\nList Keys\n");
	response = memkv_list_keys(client);
	print_memkv_result(response);
	free(response);

	// get key
	fprintf(stdout, "\nGet Key '%s'\n", key);
	response = memkv_get_key(client, key);
	print_memkv_result(response);
	free(response);

	fprintf(stdout, "\nPut Key '%s'\n", key2);
	response = memkv_put_key(client, key2, put_body);
	print_memkv_result(response);
	free(response);

	fprintf(stdout, "\nPut Key '%s'\n", key3);
	response = memkv_put_key(client, key3, put_body);
	print_memkv_result(response);
	free(response);

	fprintf(stdout, "\nPut Key '%s' (again)\n", key3);
	response = memkv_put_key(client, key3, put_body);
	print_memkv_result(response);
	free(response);

	char prefix[] = "c";

	fprintf(stdout, "\nList Keys With Prefix '%s'\n", prefix);
	response = memkv_list_keys_with_prefix(client, prefix);
	print_memkv_result(response);
	free(response);

	prefix[0] = 'a';
	fprintf(stdout, "\nList Keys With Prefix '%s'\n", prefix);
	response = memkv_list_keys_with_prefix(client, prefix);
	print_memkv_result(response);
	free(response);

	// delete key
	prefix[0] = 'c';
	fprintf(stdout, "\nMaking a delete Key Request with Prefix '%s'\n", prefix);
	response = memkv_delete_keys_with_prefix(client, prefix);
	print_memkv_result(response);
	free(response);

	// delete all keys
	fprintf(stdout, "\nDelete all Keys\n");
	response = memkv_delete_all_keys(client);
	print_memkv_result(response);
	free(response);

	fprintf(stdout, "\nMaking a delete Key '%s'\n", key3);
	response = memkv_delete_key(client, key3);
	print_memkv_result(response);
	free(response);

	// list key
	fprintf(stdout, "\nList Keys\n");
	response = memkv_list_keys(client);
	print_memkv_result(response);
	free(response);

	free(client);
}

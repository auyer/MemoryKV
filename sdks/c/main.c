#include <curl/curl.h>
#include <libMemoryKV.h>
#include <stdio.h>

int main(void) {

	char key[] = "c_sdk2";

	// put key
	fprintf(stdout, "\nPut Key Request\n");

	static const char put_body[] = "{"
								   " \"name\" : \"c_sdk\","
								   " \"content\" : \"json\""
								   "}";

	memkv_client *client;
	client = memkv_client_new("http://localhost:8080");

	memkv_put_key(client, key, put_body);

	fprintf(stdout, "\nList Keys Request\n");
	// list key
	memkv_list_keys(client);

	// get key
	fprintf(stdout, "\nMaking a Get Key Request\n");

	memkv_get_key(client, key);

	// delete key
	fprintf(stdout, "\nMaking a delete Key Request\n");

	memkv_delete_key(client, key);
}

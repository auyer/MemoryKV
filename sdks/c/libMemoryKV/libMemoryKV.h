#include <stdbool.h>

typedef struct memkv_client {
	char *host;
} memkv_client;

// memkv_result stores a success boolean to indicate if the request was successfull or not.
// the success variable should be checked before reading results
// in case of success, the result will be stored in the result attribure `memkv_result->result`
// in case of error, the error reason will be stored in the error attribure `memkv_result->error`
typedef struct {
	bool success : 1;
	union {
		char *result;
		char *error;
	};
} memkv_result;

void memkv_init_client(struct memkv_client *client, char *host);

memkv_client *memkv_client_new(char *host);

memkv_result *memkv_get_key(struct memkv_client *client, const char *key);

memkv_result *memkv_delete_key(struct memkv_client *client, const char *key);

memkv_result *memkv_put_key(struct memkv_client *client, const char *key, const char *put_body);

memkv_result *memkv_list_keys(struct memkv_client *client);

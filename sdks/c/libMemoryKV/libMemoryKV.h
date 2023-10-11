#include <stdbool.h>

// memkv_client stores the configurations used to connect to the MemoryKV server
// use `memkv_client_new` to create one, or `memkv_init_client` to initialize an existing instance
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

// memkv_client_new creates a new memkv_client instance with a host url pre filled
memkv_client *memkv_client_new(char *host);

// memkv_init_client initializes an existing memkv_client with a provided URL
void memkv_init_client(struct memkv_client *client, char *host);

// memkv_get_key fetches the value of a key from the MemoryKV server
memkv_result *memkv_get_key(struct memkv_client *client, const char *key);

// memkv_delete_key deletes a key from the MemoryKV server, and return a value if it was deleted
memkv_result *memkv_delete_key(struct memkv_client *client, const char *key);

// memkv_put_key puts a key value pair in the MemoryKV server, and return a value if there was one in the key beafore
memkv_result *memkv_put_key(struct memkv_client *client, const char *key, const char *put_body);

// memkv_list_keys lists all the keys in the MemoryKV server
memkv_result *memkv_list_keys(struct memkv_client *client);

// memkv_list_keys_with_prefix lists all the keys in the MemoryKV server with a given prefix
memkv_result *memkv_list_keys_with_prefix(struct memkv_client *client, const char *key_prefix);

// memkv_delete_key deletes a key from the MemoryKV server, and return the keys that were deleted
memkv_result *memkv_delete_keys_with_prefix(struct memkv_client *client, const char *key_prefix);

// memkv_delete_all_keys deletes all keys from the MemoryKV server, and returns list of deletes keys
memkv_result *memkv_delete_all_keys(struct memkv_client *client);
typedef struct memkv_client {
	char *host;
} memkv_client;

void memkv_init_client(struct memkv_client *client, char *host);

memkv_client *memkv_client_new(char *host);

char *memkv_get_key(struct memkv_client *client, const char *key);

char *memkv_delete_key(struct memkv_client *client, const char *key);

char *memkv_put_key(struct memkv_client *client, const char *key, const char *put_body);

char *memkv_list_keys(struct memkv_client *client);

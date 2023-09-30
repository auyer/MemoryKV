#include <curl/curl.h>
#include <libMemoryKV.h>
#include <stdio.h>

int main(void) {

  char key[] = "c_sdk2";

  // put key
  fprintf(stdout, "\nPut Keys Request\n");

  static const char put_body[] = "{"
                                 " \"name\" : \"c_sdk\","
                                 " \"content\" : \"json\""
                                 "}";
  put_key(key, put_body);

  list_keys();

  // get key
  fprintf(stdout, "\nMaking a Get Key Request\n");

  get_key(key);

  // delete key
  fprintf(stdout, "\nMaking a delete Key Request\n");

  delete_key(key);
}

#ifndef _TUBE_CURL_H_
#define _TUBE_CURL_H_

#include <curl/curl.h>
#include <json/json.h>

struct tube_curl_payload {
    char *payload;
    size_t size;
    json_object *json;
};

void tube_curl_paylaod_free(struct tube_curl_payload *p);

int tube_curl_post(char* url, void* tube_curl, void *postfields, struct tube_curl_payload* p);
int tube_curl_read(char* url, void* tube_curl, struct tube_curl_payload* p);

void* tube_curl_init(char *gateway_id, char *apikey);
void tube_curl_cleanup(void* tube_curl);

#endif //#ifndef _TUBE_CURL_H_

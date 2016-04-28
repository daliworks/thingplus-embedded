#ifndef _TCURL_H_
#define _TCURL_H_

#include <curl/curl.h>
#include <json/json.h>

struct tcurl_payload {
    char *payload;
    size_t size;
    json_object *json;
};

void tcurl_payload_free(struct tcurl_payload *p);

int tcurl_post(char* url, void* tube_curl, void *postfields, struct tcurl_payload* p);
int tcurl_read(char* url, void* tube_curl, struct tcurl_payload* p);

void* tcurl_init(char *gateway_id, char *apikey);
void tcurl_cleanup(void* tube_curl);

#endif //#ifndef _TCURL_H_

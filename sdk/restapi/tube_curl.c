#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <json/json.h>

#include "tube_curl.h"

#define TUBE_CURL_DEBUG_PAYLOAD_PRINT

static size_t _curl_callback (void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct tube_curl_payload* p = (struct tube_curl_payload*)userp;

	p->payload = (char*)realloc(p->payload, p->size + realsize + 1);
	if (p->payload == NULL) {
		fprintf(stderr, "[CURL] realloc failed\n");
		return -1;
	}

	memcpy(&(p->payload[p->size]), contents, realsize);
	p->size += realsize;
	p->payload[p->size] = 0;

	return realsize;
}


static int _curl_exec(char* url, void* tube_curl, void *postfields, struct tube_curl_payload* p) 
{
	int ret = -1;
	struct curl_slist* headers = tube_curl;

	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		fprintf(stderr, "[CURL] curl_easy_init failed\n");
		goto err_curl_ease_init;
	}

	memset(p, 0, sizeof(struct tube_curl_payload));
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)p);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_callback);
	if (postfields)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK || p->payload == NULL) {
		ret *= -1;
		fprintf(stderr, "[CURL] curl_easy_perform failed\n");
		fprintf(stderr, "[CURL] url:%s\n", url);
		goto err_curl_easy_perform;
	}

#ifdef TUBE_CURL_DEBUG_PAYLOAD_PRINT
	printf("%s\n\n", p->payload);
#endif

	p->json = json_tokener_parse(p->payload);
	if (p->json == NULL) {
		fprintf(stderr, "[CURL] json_tokener_parse failed\n");
		goto err_json_tokener_parse;
	}
	ret = 0;

err_json_tokener_parse:
err_curl_easy_perform:
	curl_easy_cleanup(curl);

err_curl_ease_init:
	return ret;
}

void tube_curl_paylaod_free(struct tube_curl_payload *p)
{
	if (!p)
		return;

	if (p->json)
		json_object_put(p->json);

	if (p->payload)
		free(p->payload);
}

int tube_curl_post(char* url, void* tube_curl, void *postfields, struct tube_curl_payload* p)
{
	return _curl_exec(url, tube_curl, postfields, p);
}

int tube_curl_read(char* url, void* tube_curl, struct tube_curl_payload* p) 
{
	return _curl_exec(url, tube_curl, NULL, p);
}

void tube_curl_cleanup(void* tube_curl)
{
	struct curl_slist* headers = (struct curl_slist*)tube_curl;
	if (!headers)
		return;

	curl_slist_free_all(headers);
}

void* tube_curl_init(char *gateway_id, char *apikey)
{
	struct curl_slist* headers = NULL;

	char *curl_header_username = calloc(1, strlen("username:") + strlen(gateway_id) + 1);
	sprintf(curl_header_username, "%s%s", "username:", gateway_id);
	headers = curl_slist_append(headers, curl_header_username);
	free(curl_header_username);

	char *curl_header_apikey = calloc(1, strlen("apikey:") + strlen(apikey) + 1);
	sprintf(curl_header_apikey, "%s%s", "apikey:", apikey);
	headers = curl_slist_append(headers, curl_header_apikey);
	free(curl_header_apikey);

	headers = curl_slist_append(headers, "Content-Type: application/json");

	return (void*)headers;
}

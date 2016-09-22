#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "tcurl.h"

//#define TUBE_CURL_DEBUG_PAYLOAD_PRINT

static size_t _curl_callback (void *contents, size_t size, size_t nmemb, void *userp)
{
	if (!contents || !size || !nmemb)
		return -1;

	size_t realsize = size * nmemb;
	struct tcurl_payload* p = (struct tcurl_payload*)userp;
	int realloc_size = p->size + realsize + 1;

	p->payload = (char*)realloc(p->payload, realloc_size);
	if (p->payload == NULL) {
		fprintf(stdout, "[CURL] realloc failed. size:%d\n",
			realloc_size);
		return -1;
	}

	memcpy(&(p->payload[p->size]), contents, realsize);
	p->size += realsize;
	p->payload[p->size] = 0;

	return realsize;
}

static int _curl_exec(char* url, void* tube_curl, void *postfields, struct tcurl_payload* p) 
{
	int ret = -1;
	struct curl_slist* headers = tube_curl;

	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		fprintf(stdout, "[CURL] curl_easy_init failed\n");
		goto err_curl_ease_init;
	}

	memset(p, 0, sizeof(struct tcurl_payload));
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)p);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_callback);
	if (postfields)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK || p->payload == NULL) {
		fprintf(stdout, "[CURL] curl_easy_perform failed. ret:%d\n", ret);
		fprintf(stdout,  "[CURL] url:%s\n", url);
		ret *= -1;
		goto err_curl_easy_perform;
	}

#ifdef TUBE_CURL_DEBUG_PAYLOAD_PRINT
	printf("%s\n\n", p->payload);
#endif

	p->json = json_tokener_parse(p->payload);
	if (p->json == NULL) {
		fprintf(stdout, "[CURL] json_tokener_parse failed\n");
		ret = -1;
		goto err_json_tokener_parse;
	}
	ret = 0;

err_json_tokener_parse:
err_curl_easy_perform:
	curl_easy_cleanup(curl);

err_curl_ease_init:
	return ret;
}

void tcurl_payload_free(struct tcurl_payload *p)
{
	if (!p)
		return;

	if (p->json)
		json_object_put(p->json);

	if (p->payload)
		free(p->payload);
}

int tcurl_post(char* url, void* tube_curl, void *postfields, struct tcurl_payload* p)
{
	fprintf(stdout, "[TUBE_CURL] POST %s\n", url);

	memset(p, 0, sizeof(struct tcurl_payload));
	if (_curl_exec(url, tube_curl, postfields, p) < 0) {
		fprintf(stdout, "[TUBE_CUR] _curl_exec failed\n");
		return -1;
	}

	int response = atoi(p->payload) * -1;
	if (response < 0) {
		fprintf(stdout, "[TUBE_CURL] server response %d. msg:%s\n", response, p->payload);

		tcurl_payload_free(p);
		memset(p, 0, sizeof(struct tcurl_payload));
		return response;
	}

	return 0;
}

int tcurl_read(char* url, void* tube_curl, struct tcurl_payload* p) 
{
	fprintf(stdout, "[TUBE_CURL] READ %s\n", url);

	memset(p, 0, sizeof(struct tcurl_payload));
	if (_curl_exec(url, tube_curl, NULL, p) < 0) {
		fprintf(stdout, "[TUBE_CUR] _curl_exec failed\n");
		return -1;
	}

	json_object *status_object = NULL;
	if (json_object_object_get_ex(p->json, "statusCode", &status_object)) {
		fprintf(stdout, "[TUBE_CURL] server response %d. \n", json_object_get_int(status_object));

		json_object *message_object = NULL;
		if (json_object_object_get_ex(p->json, "message", &message_object)) {
			fprintf(stdout, "[TUBE_CURL] server response message:%s. \n", json_object_get_string(message_object));
		};

		tcurl_payload_free(p);
		memset(p, 0, sizeof(struct tcurl_payload));
		return -1;
	}

	return 0;
}

void tcurl_cleanup(void* tube_curl)
{
	if (tube_curl == NULL)
		return;

	curl_global_cleanup();

	struct curl_slist* headers = (struct curl_slist*)tube_curl;
	if (!headers)
		return;

	curl_slist_free_all(headers);
}

void* tcurl_init(char *gateway_id, char *apikey)
{
	if (gateway_id == NULL) {
		return NULL;
	}

	if (apikey == NULL) {
		return NULL;
	}

	struct curl_slist* headers = NULL;

	curl_global_init(CURL_GLOBAL_ALL);

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

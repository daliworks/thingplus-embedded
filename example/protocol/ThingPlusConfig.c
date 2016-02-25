#include <stdlib.h>
#include <stdio.h>

#include "cJSON.h"
#include "ThingPlusConfig.h"

#define ERROR_LOG(fmt, args...) fprintf(stderr, "ERROR: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)


static cJSON* _config_json_parse(char *filename)
{
	long len;
	char *data;
	cJSON *json;
	FILE *f;

	f=fopen(filename,"rb");
	fseek(f,0,SEEK_END); len=ftell(f); fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);
	fread(data,1,len,f);

	json=cJSON_Parse(data);
	if (!json) {
		ERROR_LOG("Config Error before: [%s]\n",cJSON_GetErrorPtr());
	}
	free(data);
	fclose(f);
	return json;
}

struct config* config_init(char *filename)
{
	cJSON *json = _config_json_parse(filename);
	if (json == NULL) {
		ERROR_LOG("_config_json_parse failed. filename:%s\n", filename);
		return NULL;
	}

	struct config *c = calloc(1, sizeof(struct config));
	if (c == NULL) {
		ERROR_LOG("calloc failed. size:%d\n", (int)sizeof(struct config));
		cJSON_Delete(json);
		return NULL;
	}

	c->json = json;
	c->gateway_id = cJSON_GetObjectItem(json, "gatewayId")->valuestring;
	c->hostname = cJSON_GetObjectItem(json, "hostname")->valuestring;
	c->host = cJSON_GetObjectItem(json, "host")->valuestring;
	c->port = cJSON_GetObjectItem(json, "port")->valueint;
	c->username = cJSON_GetObjectItem(json, "username")->valuestring;
	c->password = cJSON_GetObjectItem(json, "password")->valuestring;
	c->ca_file = cJSON_GetObjectItem(json, "caFile")->valuestring;

	return c;
}

void config_cleanup(struct config* c) 
{
	if (c == NULL) {
		return;
	}

	cJSON_Delete(c->json);
	free(c);
}


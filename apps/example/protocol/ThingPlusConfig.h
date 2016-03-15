#ifndef _THINGPLUSCONFIG_H_
#define _THINGPLUSCONFIG_H_

struct config {
	cJSON *json;
	char *gateway_id;
	char *hostname;
	char *host;
	int port;
	char *username;
	char *password;
	char *ca_file;
};

void config_cleanup(struct config* c) ;
struct config* config_init(char *filename);

#endif //#ifndef _THINGPLUSCONFIG_H_

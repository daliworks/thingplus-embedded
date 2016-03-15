#ifndef _TUBE_URL_H_
#define _TUBE_URL_H_

#include <stdarg.h>

enum tube_http_url {			// tube_http_url_get arg list
	TUBE_HTTP_URL_GATEWAY_INFO,	// char *gateway_id;
	TUBE_HTTP_URL_GATEWAY_MODEL,	// int model_id;
	TUBE_HTTP_URL_DEVICE_REGISTER,	// char *gateway_id;
	TUBE_HTTP_URL_SENSOR_DRIVERS,	// char *driver_name;
	TUBE_HTTP_URL_SENSOR_REGISTER,	// char *gateway_id;
	TUBE_HTTP_URL_DEVICE_INFO,	// char *gateway_id, char *device_id;
};

//char *tube_http_url_get(enum tube_http_url r, void *arg);
char *tube_http_url_get(enum tube_http_url r, ...);
void tube_http_url_put(char *url);

#endif //#ifndef _TUBE_URL_H_

#ifndef _URL_H_
#define _URL_H_

#include <stdarg.h>

enum url_index {			// url_get arg list
	URL_INDEX_GATEWAY_INFO,	// char *gateway_id;
	URL_INDEX_GATEWAY_MODEL,	// int model_id;
	URL_INDEX_DEVICE_REGISTER,	// char *gateway_id;
	URL_INDEX_SENSOR_DRIVERS,	// char *driver_name;
	URL_INDEX_SENSOR_REGISTER,	// char *gateway_id;
	URL_INDEX_DEVICE_INFO,	// char *gateway_id, char *device_id;
};

char *url_get(enum url_index r, ...);
void url_put(char *url);

#endif //#ifndef _URL_H_

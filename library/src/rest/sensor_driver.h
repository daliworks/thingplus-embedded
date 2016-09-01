#ifndef _SENSOR_DRIVER_H_
#define _SENSOR_DRIVER_H_

#include <curl/curl.h>
#include <json-c/json.h>

char* sensor_driver_id_template_get(void* tube_curl, char *driver_name);
void sensor_driver_cleanup(void);

#endif //#ifndef _SENSOR_DRIVER_H_

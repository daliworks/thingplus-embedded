#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _THINGPLUS_H_
#define _THINGPLUS_H_

#include <stdbool.h>
#include <stdint.h>

#include <thingplus_types.h>

#define THINGPLUS_ID_LENGTH 128

int thingplus_gatewayinfo(void *t, struct thingplus_gatewayinfo*);
int thingplus_deviceinfo(void *t, char *, struct thingplus_device *);

int thingplus_sensor_register(void *t, char *name, int uid, char* type, char* device_id, 
		char sensor_id[THINGPLUS_ID_LENGTH]);
int thingplus_device_register(void* instance, char* name, int uid, char* device_model_id, 
		char device_id[THINGPLUS_ID_LENGTH]);

int thingplus_value_publish(void *_t, int nr_value, struct thingplus_value *values);
int thingplus_status_publish(void *t, int nr_status, struct thingplus_status *status);

int thingplus_disconnect(void *t);
int thingplus_connect(void *t, char *ca_file, int keepalive);

void thingplus_callback_set(void *_t, struct thingplus_callback *callback, void *callback_arg);

void *thingplus_init(char *gw_id, char *apikey, char *mqtt_url, char *restapi_url);
void thingplus_cleanup(void *t);

#endif //#ifndef _THINGPLUS_H_

#ifdef __cplusplus
}
#endif

#ifndef _MQTT_H_
#define _MQTT_H_

#include <thingplus_types.h>

#define THINGPLUS_ID_LENGTH_MAX 128

void *mqtt_init(char *gw_id, char *apikey, char *mqtt_url);
void mqtt_cleanup(void *_t);

int mqtt_connect(void *_t, int port, char *ca_file, int keepalive);
int mqtt_disconnect(void *_t);

void mqtt_callback_set(void *_t, struct thingplus_callback *callback, void *callback_arg);

int mqtt_status_publish(void* _t, int nr_thing, struct thingplus_status *things);
int mqtt_value_publish(void *_t, int nr_value, struct thingplus_value *values);


#endif //#ifndef_MQTT_H_

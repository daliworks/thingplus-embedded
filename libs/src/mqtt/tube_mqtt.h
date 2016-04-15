#ifndef _TUBE_MQTT_H_
#define _TUBE_MQTT_H_

#include <time.h>

enum tube_mqtt_error {
	TUBE_MQTT_ERROR_SUCCESS = 0,
	TUBE_MQTT_ERROR_INVALID_ARGUMENT = -1,
	TUBE_MQTT_ERROR_NOT_CONNECTED = -2,
	TUBE_MQTT_ERROR_IDENTIFICATION = -3,
	TUBE_MQTT_ERROR_BROKER = -4,
	TUBE_MQTT_ERROR_UNKNOWN = -5,
	TUBE_MQTT_ERROR_MEM_ALLOC = -6,
};

struct tube_mqtt_callback {
	void (*connect)(void* arg, enum tube_mqtt_error result);
	//void (*disconnect)(void *id);
	//int (*pub)(void* handle, char *topic, char *message);
	//int (*sub)(void* handle, char *topic, void (*callback)(char *msg, int len));
};

/*
 * dm operation
 */
struct tube_dm_ops {
	void (*report_interval_update)(int report_interval);
	void (*time_update)(int time);
};


enum tube_mqtt_thing_status{
	TUBE_STATUS_ON,
	TUBE_STATUS_OFF,
};

/* gateway or sensor */
struct tube_thing_status {
	char* id;
	enum tube_mqtt_thing_status status;
	int valid_duration_sec;
};

struct tube_thing_value {
	char* value;
	unsigned long long time_ms;
};

struct tube_thing_values {
	char* id;
	int nr_value;
	struct tube_thing_value *value;
};

enum tube_mqtt_error tube_mqtt_status_send(void *instance, int nr_thing, struct tube_thing_status *things);
enum tube_mqtt_error tube_mqtt_single_value_send(void* instance, char* id, struct tube_thing_value *value);
enum tube_mqtt_error tube_mqtt_values_send(void* instance, int nr_thing, struct tube_thing_values *values);

void* tube_mqtt_connect(char* gw_id, char* apikey, char* ca_file, int report_interval, struct tube_mqtt_callback *mqtt_cb, void* cb_arg, char* logfile);
void tube_mqtt_disconnect(void* t);

//bool tube_mqtt_value_send()

#endif //#ifndef _TUBE_MQTT_H_

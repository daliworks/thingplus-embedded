#ifndef _THINGPLUS_H_
#define _THINGPLUS_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum thingplus_error {
	THINGPLUS_ERR_SUCCESS = 0,
	THINGPLUS_ERR_INVALID_ARGUMENT = -1,
	THINGPLUS_ERR_NOT_CONNECTED = -2,
	THINGPLUS_ERR_IDENTIFICATION = -3,
	THINGPLUS_ERR_SERVER = -4,
	THINGPLUS_ERR_NOMEM = -5,
	THINGPLUS_ERR_SYSCALL = -6,
	THINGPLUS_ERR_MQTT_PROTOCOL = -7,
	THINGPLUS_ERR_AUTHORIZATION = -8,
	THINGPLUS_ERR_UNKNOWN = -99,
} thingplus_error_e;

/* gateway or sensor */
struct thingplus_status {
	char* id;
	enum { 
		THINGPLUS_STATUS_ON,
		THINGPLUS_STATUS_OFF,
	}status;
	int valid_duration_sec;
};

struct thingplus_value {
	char* value;
	uint64_t time_ms;
};

struct thingplus_values {
	char* id;

	int nr_value;
	struct thingplus_value *value;
};

struct thingplus_callback_result {
	bool error;
	char *message; /*error message or result */
};

struct thingplus_callback {
	void (*connected)(void *cb_arg, thingplus_error_e err);
	void (*disconnected)(void *cb_arg, bool force);

	bool (*actuating)(void *cb_arg, const char *id, const char *cmd, const char *options, char **return_message);
	bool (*timesync)(void *cb_arg, uint64_t time, char **return_message);
	bool (*property_set)(void *cb_arg, int report_interval, char **return_message);
	bool (*poweroff)(void *cb_arg, char **return_message);
	bool (*reboot)(void *cb_arg, char **return_message);
	bool (*restart)(void *cb_arg, char **return_message);
	bool (*upgrade)(void *cb_arg, char **return_message);
	bool (*version)(void *cb_arg, char **return_message);
};

thingplus_error_e thingplus_values_publish(void* instance, int nr_thing, struct thingplus_values *values);
thingplus_error_e thingplus_single_value_publish(void* instance, char* id, struct thingplus_value *value);
thingplus_error_e thingplus_status_publish(void *instance, int nr_thing, struct thingplus_status *things);

thingplus_error_e thingplus_mqtt_loop(void *instance);

thingplus_error_e thingplus_connect(void *intance);
enum thingplus_error thingplus_connect_async(void *instance);
thingplus_error_e thingplus_disconnect(void *intance);

void* thingplus_init(char* gw_id, char* apikey, char* ca, int keepalive_sec, struct thingplus_callback* callback, void* cb_arg);
void thingplus_cleanup(void *instance);

#endif //#ifndef _THINGPLUS_H_


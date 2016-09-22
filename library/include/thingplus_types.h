#ifndef _THINGPLUS_TYPES_H_
#define _THINGPLUS_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#define THINGPLUS_DM_RESULT_LENGTH 2048

enum thingplus_error {
	THINGPLUS_ERR_SUCCESS = 0,
	THINGPLUS_ERR_INVALID_ARGUMENT = -1,
	THINGPLUS_ERR_NOT_CONNECTED = -2,
	THINGPLUS_ERR_IDENTIFICATION = -3,
	THINGPLUS_ERR_SERVER = -4,
	THINGPLUS_ERR_NOMEM = -5,
	THINGPLUS_ERR_SYSCALL = -6,
	THINGPLUS_ERR_MQTT_PROTOCOL = -7,
	THINGPLUS_ERR_AUTHORIZATION = -8,
	THINGPLUS_ERR_NETWORK = -9,
	THINGPLUS_ERR_UNEXPECTED = -10,
	THINGPLUS_ERR_INVALID_TYPE = -11,
	THINGPLUS_ERR_DUPLICATED = -12,
	THINGPLUS_ERR_CURL_POST = -13,
	THINGPLUS_ERR_CURL_READ = -14,
	THINGPLUS_ERR_NOT_DISOVERABLE = -15,
	THINGPLUS_ERR_UNKNOWN = -99,
};

#define THINGPLUS_NAME_LENGTH	512
struct thingplus_sensor {
	char id[THINGPLUS_NAME_LENGTH];//
	char network[THINGPLUS_NAME_LENGTH];//
	char driver_name[THINGPLUS_NAME_LENGTH];//
	char model[THINGPLUS_NAME_LENGTH];//
	char type[THINGPLUS_NAME_LENGTH];//
	char category[THINGPLUS_NAME_LENGTH];//
	char name[THINGPLUS_NAME_LENGTH];//
	char device_id[THINGPLUS_NAME_LENGTH];//
	char owner[THINGPLUS_NAME_LENGTH];//
	time_t mtime;//
	time_t ctime;//
};

struct thingplus_device {
	char name[THINGPLUS_NAME_LENGTH];
	char model[THINGPLUS_NAME_LENGTH];
	char owner[THINGPLUS_NAME_LENGTH];
	time_t mtime;
	time_t ctime;
	char id[THINGPLUS_NAME_LENGTH];
};

struct thingplus_device_short_info { 
	char id[THINGPLUS_NAME_LENGTH];
	char model[THINGPLUS_NAME_LENGTH];
};

struct thingplus_gateway {
	char name[THINGPLUS_NAME_LENGTH];
	char id[THINGPLUS_NAME_LENGTH];
	bool discoverable;

	int nr_devices;
	struct thingplus_device_short_info device_sinfo;
	char** devices;

	int nr_sensors;
	char** sensors;

	time_t mtime;
	time_t ctime;
	int site;
	int model;
};


struct thingplus_value {
	char *id;
	char *value;
	uint64_t time_ms;
};

struct thingplus_status {
	char *id;
	enum {
		THINGPLUS_STATUS_ON,
		THINGPLUS_STATUS_OFF
	} status;
	int timeout_ms;
};

struct thingplus_callback {
	void (*connected)(void *cb_arg, enum thingplus_error error);
	void (*disconnected)(void *cb_arg, enum thingplus_error error);

	bool (*actuating)(void *cb_arg, const char *id, const char *cmd, const char *options, 
			char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*timesync)(void *cb_arg, uint64_t time, 
			char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*property_set)(void *cb_arg, int report_interval, 
			char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*poweroff)(void *cb_arg, char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*reboot)(void *cb_arg, char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*restart)(void *cb_arg, char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*upgrade)(void *cb_arg, char result_message[THINGPLUS_DM_RESULT_LENGTH]);
	bool (*version)(void *cb_arg, char result_message[THINGPLUS_DM_RESULT_LENGTH]);
};

#endif //#ifndef _THINGPLUS_TYPES_H_

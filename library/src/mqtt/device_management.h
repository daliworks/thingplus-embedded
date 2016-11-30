#ifndef _DEVICE_MANAGEMENT_H_
#define _DEVICE_MANAGEMENT_H_

#include <stdbool.h>
#include <stdint.h>

#define DEVICE_MANAGEMENT_RESULT_LENGTH 2048

struct device_management_ops
{
	bool (*actuating)(void *cb_arg, const char *id, const char *cmd, const char *options, 
			char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*timesync)(void *cb_arg, uint64_t time, 
			char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*property_set)(void *cb_arg, int report_interval, 
			char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*poweroff)(void *cb_arg, char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*reboot)(void *cb_arg, char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*restart)(void *cb_arg, char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*upgrade)(void *cb_arg, char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
	bool (*version)(void *cb_arg, char result_message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
};

void *device_management_init(struct device_management_ops *ops, void *arg);
void device_management_cleanup(void *d);

char* device_management_do(void *d, char *payload);
void device_management_result_free(char *result);

#endif //#define _DEVICE_MANAGEMENT_H_

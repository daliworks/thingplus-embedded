#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#include "device_management.h"
#include "packetize.h"

struct device_management {
	struct device_management_ops ops;
	void *arg;
};

static bool _version(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.version == NULL)
		return false;

	return d->ops.version(d->arg, message);
}

static bool _upgrade(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.upgrade == NULL)
		return false;

	return d->ops.upgrade(d->arg, message);
}

static bool _restart(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.restart == NULL)
		return false;

	return d->ops.restart(d->arg, message);
}

static bool _reboot(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.reboot == NULL)
		return false;

	return d->ops.reboot(d->arg, message);
}

static bool _poweroff(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.poweroff == NULL)
		return false;

	return d->ops.poweroff(d->arg, message);
}

static bool _property_set(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.property_set == NULL)
		return false;

	json_object *interval_object;
	if (!json_object_object_get_ex(params_object, "reportInterval", &interval_object)) {
		fprintf(stdout, "[MQTT] time get failed\n");
		return false;
	}
	int interval = json_object_get_int64(interval_object);

	return d->ops.property_set(d->arg, interval, message);
}

static bool _timesync(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.timesync == NULL)
		return false;

	json_object *time_object;
	if (!json_object_object_get_ex(params_object, "time", &time_object)) {
		fprintf(stdout, "[MQTT] time get failed\n");
		return false;
	}
	uint64_t server_time = json_object_get_int64(time_object);

	return d->ops.timesync(d->arg, server_time, message);
}

static bool _actuating(struct device_management *d, json_object* params_object, 
		char message[DEVICE_MANAGEMENT_RESULT_LENGTH])
{
	if (d->ops.actuating == NULL)
		return false;

	json_object *id_object;
	if (!json_object_object_get_ex(params_object, "id", &id_object)) {
		fprintf(stdout, "[MQTT] id get failed\n");
		return false;
	}

	json_object *cmd_object;
	if (!json_object_object_get_ex(params_object, "cmd", &cmd_object)) {
		fprintf(stdout, "[MQTT] cmd get failed\n");
		return false;
	}

	json_object *options_object;
	if (!json_object_object_get_ex(params_object, "options", &options_object)) {
		fprintf(stdout, "[MQTT] options get failed\n");
		return false;
	}

	return d->ops.actuating(d->arg, 
			json_object_get_string(id_object), 
			json_object_get_string(cmd_object), 
			json_object_get_string(options_object),
			message);
}

static const struct {
	char *name;
	bool (*device_management)(struct device_management* d, json_object* params_object, char message[DEVICE_MANAGEMENT_RESULT_LENGTH]);
}_services_ops[] = {
	{.name = "controlActuator", .device_management = _actuating},
	{.name = "timeSync", .device_management = _timesync},
	{.name = "setProperty", .device_management = _property_set},
	{.name = "poweroff", .device_management = _poweroff},
	{.name = "reboot", .device_management = _reboot},
	{.name = "restart", .device_management = _restart},
	{.name = "swUpdate", .device_management = _upgrade},
	{.name = "swInfo", .device_management = _version},
};
static const int _nr_service_ops = sizeof(_services_ops)/sizeof(_services_ops[0]);

char *device_management_do(void *d, char *payload)
{
	if (d == NULL) {
		return NULL;
	}

	if (payload == NULL) {
		return NULL;
	}

	json_object *json = json_tokener_parse(payload);
	if (json == NULL) {
		fprintf(stdout, "[MQTT] json_tokener_parse failed\n");
		return NULL;
	}

	json_object *id_object = NULL;
	const char *id = NULL;
	if (!json_object_object_get_ex(json, "id", &id_object)) {
		fprintf(stdout, "[MQTT] id get failed\n");
		return NULL;
	}
	id = json_object_get_string(id_object);

	char *result = NULL;
	int result_size = 0;

	const char *method = NULL;
	json_object *method_object = NULL;
	if (!json_object_object_get_ex(json, "method", &method_object)) {
		fprintf(stdout, "[MQTT] method get failed\n");

		result_size = packetize_payload(&result, result_size, 
				"{\"id\":\"%s\",\"error\":\"code\":-32601,\"message\":\"%s\"}", 
				id, "missing method field");
		return result;
	}
	method = json_object_get_string(method_object);

	json_object *params_object = NULL;
	if (!json_object_object_get_ex(json, "params", &params_object)) {
		fprintf(stdout, "[MQTT] params get failed\n");
		params_object = NULL;
	}

	int i = 0;
	bool ret = NULL;
	char message[DEVICE_MANAGEMENT_RESULT_LENGTH] = {0,};
	for (i=0; i<_nr_service_ops; i++) {
		if (strcmp(_services_ops[i].name, method))
			continue;
		ret = _services_ops[i].device_management(d, params_object, message);
		break;
	}

	if (ret) {
		result_size = packetize_payload(&result, result_size, 
				"{\"id\":\"%s\",\"result\":\"%s\"}", id, message);
	}
	else {
		result_size = packetize_payload(&result, result_size, 
				"{\"id\":\"%s\",\"error\":\"code\":-32000,\"message\":\"%s\"}", 
				id, message);
	}

	return result;
}

void device_management_result_free(char *result)
{
	if (!result)
		return;

	packetize_free(result);
}

void *device_management_init(struct device_management_ops *ops, void *arg)
{
	struct device_management *d = calloc(1, sizeof(struct device_management));
	if (d == NULL) {
		fprintf(stdout, "[device_management] calloc failed. size:%ld\n", 
			sizeof(struct device_management));
		return NULL;
	}

	d->ops = *ops;
	d->arg = arg;

	return d;
}

void device_management_cleanup(void *d)
{
	if (!d)
		return;
	free(d);
}

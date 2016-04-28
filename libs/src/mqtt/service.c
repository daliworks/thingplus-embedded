#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "service.h"

#include "log/tube_log.h"
#include "tube_mqtt.h"

struct service {
	struct thingplus_callback cb;
	void *cb_arg;
};

static bool _version(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.version == NULL)
		return false;

	return s->cb.version(s->cb_arg, message);
}

static bool _upgrade(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.upgrade == NULL)
		return false;

	return s->cb.upgrade(s->cb_arg, message);
}

static bool _restart(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.restart == NULL)
		return false;

	return s->cb.restart(s->cb_arg, message);
}

static bool _reboot(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.reboot == NULL)
		return false;

	return s->cb.reboot(s->cb_arg, message);
}

static bool _poweroff(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.poweroff == NULL)
		return false;

	return s->cb.poweroff(s->cb_arg, message);
}

static bool _property_set(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.property_set == NULL)
		return false;

	json_object *interval_object;
	if (!json_object_object_get_ex(params_object, "reportInterval", &interval_object)) {
		tube_log_info("[MQTT] time get failed\n");
		return false;
	}
	int interval = json_object_get_int64(interval_object);

	return s->cb.property_set(s->cb_arg, interval, message);
}

static bool _timesync(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.timesync == NULL)
		return false;

	json_object *time_object;
	if (!json_object_object_get_ex(params_object, "time", &time_object)) {
		tube_log_info("[MQTT] time get failed\n");
		return false;
	}
	uint64_t server_time = json_object_get_int64(time_object);

	return s->cb.timesync(s->cb_arg, server_time, message);
}

static bool _actuating(struct service *s, json_object* params_object, char **message)
{
	if (s->cb.actuating == NULL)
		return false;

	json_object *id_object;
	if (!json_object_object_get_ex(params_object, "id", &id_object)) {
		tube_log_info("[MQTT] id get failed\n");
		return false;
	}

	json_object *cmd_object;
	if (!json_object_object_get_ex(params_object, "cmd", &cmd_object)) {
		tube_log_info("[MQTT] cmd get failed\n");
		return false;
	}

	json_object *options_object;
	if (!json_object_object_get_ex(params_object, "options", &options_object)) {
		tube_log_info("[MQTT] options get failed\n");
		return false;
	}

	return s->cb.actuating(s->cb_arg, 
			json_object_get_string(id_object), 
			json_object_get_string(cmd_object), 
			json_object_get_string(options_object),
			message);
}

static const struct {
	char *name;
	bool (*service)(struct service* s, json_object* params_object, char **message);
}_services_ops[] = {
	{.name = "controlActuator", .service = _actuating},
	{.name = "timeSync", .service = _timesync},
	{.name = "setProperty", .service = _property_set},
	{.name = "poweroff", .service = _poweroff},
	{.name = "reboot", .service = _reboot},
	{.name = "restart", .service = _restart},
	{.name = "swUpdate", .service = _upgrade},
	{.name = "swInfo", .service = _version},
};
static const int _nr_service_ops = sizeof(_services_ops)/sizeof(_services_ops[0]);

char *service_do(void *s, char *payload)
{
	if (s == NULL) {
		return NULL;
	}

	if (payload == NULL) {
		return NULL;
	}

	json_object *json = json_tokener_parse(payload);
	if (json == NULL) {
		tube_log_error("[MQTT] json_tokener_parse failed\n");
		return;
	}

	json_object *id_object = NULL;
	const char *id = NULL;
	if (!json_object_object_get_ex(json, "id", &id_object)) {
		tube_log_error("[MQTT] id get failed\n");
		return;
	}
	id = json_object_get_string(id_object);

	const char *method = NULL;
	json_object *method_object = NULL;
	if (!json_object_object_get_ex(json, "method", &method_object)) {
		tube_log_error("[MQTT] method get failed\n");
		return;
	}
	method = json_object_get_string(method_object);

	json_object *params_object = NULL;
	if (!json_object_object_get_ex(json, "params", &params_object)) {
		tube_log_warn("[MQTT] params get failed\n");
		params_object = NULL;
	}

	int i;
	bool ret;
	char *message = NULL;
	for (i=0; i<_nr_service_ops; i++) {
		if (strcmp(_services_ops[i].name, method))
			continue;
		 ret = _services_ops[i].service(s, params_object, &message);
		 break;
	}

	char *result = NULL;
	int result_size = 0;
	if (ret) {
		result_size = reassemble_payload(&result, result_size, 
				"{\"id\":\"%s\",\"result\":\"%s\"}", id, message);
	}
	else {
		result_size = reassemble_payload(&result, result_size, 
				"{\"id\":\"%s\",\"error\":\"code\":-32000,\"message\":\"%s\"}", 
				id, message);
	}

	return result;
}

void service_result_free(char *result)
{
	if (!result)
		return;

	reassemble_free(result);
}

void *service_init(struct thingplus_callback *cb, void *cb_arg)
{
	struct service *s = calloc(1, sizeof(struct service));
	if (s == NULL) {
		tube_log_error("[SERVICE] calloc failed. size:%ld\n", 
			sizeof(struct service));
		return NULL;
	}

	s->cb = *cb;
	s->cb_arg = cb_arg;

	return ;
}

void service_cleanup(void *s)
{
	if (!s)
		return;
	free(s);
}

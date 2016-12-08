#include <fcntl.h>
#include <mosquitto.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <thingplus.h>

#include "mqtt/mqtt.h"
#include "rest/rest.h"

#define STR_OR_NULL(str)	(str) ? str : "NULL"

struct thingplus {
	void *mqtt;
	void *rest;
};

int thingplus_gatewayinfo(void *_t, struct thingplus_gateway* gwinfo)
{
	if (gwinfo == NULL) {
		fprintf(stdout, "[ERR] gwinfo is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = _t;

	return rest_gatewayinfo(t->rest, gwinfo);
}

int thingplus_deviceinfo(void *_t, char *dev_id, struct thingplus_device *dev_info)
{
	return 0;
	struct thingplus *t = _t;
	return rest_deviceinfo(t->rest, dev_id, dev_info);
}

int thingplus_sensor_register(void *_t, char *name, int uid, char* type, char* device_id, char* sensor_id)
{
	if (!name || !type || !device_id) {
		fprintf(stdout, "[ERR] argument is NULL. name:%s, type:%s, device_id:%s\n",
				STR_OR_NULL(name), STR_OR_NULL(type),
				STR_OR_NULL(device_id));
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (sensor_id == NULL) {
		fprintf(stdout, "[ERR] sensor_id is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = _t;

	return rest_sensor_register(t->rest, name, uid, type, device_id, sensor_id);
}

int thingplus_device_register(void* _t, char* name, int uid, char* device_model_id, char* device_id)
{
	if (!name || !device_model_id) {
		fprintf(stdout, "[ERR] argument is NULL. name:%s, device_model_id:%s\n",
				STR_OR_NULL(name), STR_OR_NULL(device_model_id));
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (device_id == NULL) {
		fprintf(stdout, "[ERR] device_id is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = _t;

	return rest_device_register(t->rest, name, uid, device_model_id, device_id);
}

int thingplus_value_publish(void *_t, int *mid, int nr_value, struct thingplus_value *values)
{
	if (nr_value <= 0) {
		fprintf(stdout, "invalid nr_value\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (values == NULL) {
		fprintf(stdout, "values is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = _t;

	return mqtt_value_publish(t->mqtt, mid, nr_value, values);
}

int thingplus_status_publish(void* _t, int *mid, int nr_thing, struct thingplus_status *things)
{
	if (nr_thing <= 0) {
		fprintf(stdout, "invalid nr_thing\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (things == NULL) {
		fprintf(stdout, "things is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = _t;

	return mqtt_status_publish(t->mqtt, mid, nr_thing, things);
}

int thingplus_disconnect(void *_t)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = (struct thingplus*)_t;

	return mqtt_disconnect(t->mqtt);
}

int thingplus_connect(void *_t, int port, char *ca_file, int keepalive)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}
	struct thingplus *t = (struct thingplus*)_t;

	return mqtt_connect(t->mqtt, port, ca_file, keepalive);
}

void thingplus_callback_set(void *_t, struct thingplus_callback *callback, void *callback_arg)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return;
	}
	struct thingplus *t = (struct thingplus*)_t;

	return mqtt_callback_set(t->mqtt, callback, callback_arg);
}

void *thingplus_init(char *gw_id, char *apikey, char *mqtt_url, char *rest_url)
{
	if (!gw_id || !apikey || !mqtt_url || !rest_url) {
		fprintf(stdout, "[ERR] invalid argument. gw_id:%s apikey:%s mqtt_url:%s rest_url:%s\n",
				STR_OR_NULL(gw_id), STR_OR_NULL(apikey),
				STR_OR_NULL(mqtt_url), STR_OR_NULL(rest_url));
		return NULL;
	}

	struct thingplus *t = calloc(1, sizeof(struct thingplus));
	if (t == NULL) {
		fprintf(stdout, "[ERR] calloc failed. size:%ld\n", sizeof(struct thingplus));
		return NULL;
	}

	t->mqtt = mqtt_init(gw_id, apikey, mqtt_url);
	if (!t->mqtt) {
		fprintf(stdout, "[ERR] mqtt_init failed\n");
		goto err_mqtt_init;
	}

	t->rest = rest_init(gw_id, apikey, rest_url);
	if (t->rest == NULL) {
		fprintf(stdout, "[ERR] rest_init failed\n");
		goto err_rest_init;
	}
	fprintf(stdout, "[INFO] thingplus_init gw_id:%s apikey:%s\n", gw_id, apikey);

	return  (void*)t;

err_rest_init:
	mqtt_cleanup(t->mqtt);

err_mqtt_init:
	free(t);
	return NULL;
}

void thingplus_cleanup(void *_t)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return;
	}
	struct thingplus *t = (struct thingplus*)_t;

	rest_cleanup(t->rest);
	mqtt_cleanup(t->mqtt);

	free(t);
}

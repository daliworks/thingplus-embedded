#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "tube_curl.h"
#include "thingplus_discover.h"
#include "tube_url.h"
#include "sensor_driver.h"

#include "log/tube_log.h"

struct sensor {
	const char *network;
	const char *driver_name;
	const char *model;
	const char *type;
	const char *category;
}; 

struct device_model {
	const char *id;
	const char *id_template;
	bool discoverable;

	int nr_sensors;
	struct sensor *sensors;
};

struct gateway_model {
	int nr_device_models;
	struct device_model *device_models;

	struct tube_curl_payload p;
};

struct gateway_info {
	bool discoverable;
	int model;

	int nr_devices;
	char** devices;

	int nr_sensors;
	char** sensors;

	struct tube_curl_payload p;
};

struct tdiscover {
	char* gateway_id;
	char* apikey;
	void* curl;

	struct gateway_info gw_info;
	struct gateway_model gw_model;
};

static bool _discoverable_parse(json_object *json, char *key)
{
	json_object *discoverable_object = NULL;
	if (!json_object_object_get_ex(json, key, &discoverable_object)) {
		tube_log_error("[DISCOVER] json_object_object_get_ex failed. key : %s\n", key);
		return false;
	}

	const char *discoverable = json_object_get_string(discoverable_object);
	if (strcasecmp("Y", discoverable) == 0)
		return true;

	return false;
}

static int _gw_info_model_parse(struct json_object* json)
{
	json_object *model_object = NULL;
	if (json_object_object_get_ex(json, "model", &model_object) == false) {
		tube_log_error("[DISCOVER] json_object_object_get_ex failed. key : model\n");
		return -1;
	}

	return json_object_get_int(model_object);
}

static void _gw_info_device_add(struct gateway_info *gw_info, char *device_id)
{
	int realloc_size = sizeof(char*) * (gw_info->nr_devices + 1);

	gw_info->devices = realloc(gw_info->devices, realloc_size);
	if (gw_info->devices == NULL) {
		tube_log_error("[DISCOVER] realloc failed. size:%d\n",
			realloc_size);
		return;
	}

	gw_info->devices[gw_info->nr_devices++] = strdup(device_id);
}

static int _gw_info_devices_parse(struct json_object* json, char*** devices)
{
	struct json_object* devices_object = NULL;
	if (!json_object_object_get_ex(json, "devices", &devices_object)) {
		return 0;
	}

	int nr_devices = json_object_array_length(devices_object);
	*devices = malloc(sizeof(char*) * nr_devices);

	int i = 0;
	for (i=0; i<nr_devices; i++)
		(*devices)[i] = strdup(json_object_get_string(json_object_array_get_idx(devices_object, i)));

	return nr_devices;
}

static int _gw_info_sensors_parse(struct json_object* json, char*** sensors)
{
	struct json_object* sensors_object = NULL;
	if (!json_object_object_get_ex(json, "sensors", &sensors_object)) {
		return 0;
	}

	int nr_sensors = json_object_array_length(sensors_object);
	*sensors = malloc(sizeof(char*) * nr_sensors);

	int i = 0;
	for (i=0; i<nr_sensors; i++)
		(*sensors)[i] = strdup(json_object_get_string(json_object_array_get_idx(sensors_object, i)));

	return nr_sensors;
}

static void _gw_info_parse(struct tube_curl_payload *p, struct gateway_info* gw_info)
{
	gw_info->discoverable = _discoverable_parse(p->json, "autoCreateDiscoverable");
	gw_info->model = _gw_info_model_parse(p->json);
	gw_info->nr_devices = _gw_info_devices_parse(p->json, &gw_info->devices);
	gw_info->nr_sensors = _gw_info_sensors_parse(p->json, &gw_info->sensors);
}

static void _gw_info_cleanup(struct gateway_info *gw_info)
{
	if (!gw_info)
		return;

	if (gw_info->devices) {
		int i;
		for (i=0; i<gw_info->nr_devices; i++)
			free(gw_info->devices[i]);

		free(gw_info->devices);
	}

	if (gw_info->sensors) {
		int i;
		for (i=0; i<gw_info->nr_sensors; i++)
			free(gw_info->sensors[i]);

		free(gw_info->sensors);
	}

	tube_curl_paylaod_free(&gw_info->p);
}

static int _gw_info_get(struct tube_http *t)
{
	char *url = tube_http_url_get(TUBE_HTTP_URL_GATEWAY_INFO, t->gateway_id);
	if (url == NULL) {
		tube_log_error("[DISCOVER] tube_http_url_get failed. gw_id:%s\n", t->gateway_id);
		return -1;
	}

	if (tube_curl_read(url, t->curl, (void*)&t->gw_info.p) < 0) {
		tube_log_error("[DISCOVER] tube_curl_read(TUBE_HTTP_URL_GATEWAY_INFO) failed. gw_id:%s\n", t->gateway_id);

		tube_http_url_put(url);
		return -1;
	}

	_gw_info_parse(&t->gw_info.p, &t->gw_info);

	tube_http_url_put(url);
	return 0;
}

static void _sensor_parse(struct sensor *sensor, struct json_object *sensor_object)
{
	sensor->network = json_object_get_string(json_object_object_get(sensor_object, "network"));
	sensor->driver_name = json_object_get_string(json_object_object_get(sensor_object, "driverName"));
	sensor->model = json_object_get_string(json_object_object_get(sensor_object, "model"));
	sensor->type = json_object_get_string(json_object_object_get(sensor_object, "type"));
	sensor->category = json_object_get_string(json_object_object_get(sensor_object, "category"));
}

static void _device_model_parse(struct device_model *device_model, struct json_object *device_model_object)
{
	device_model->id = json_object_get_string(json_object_object_get(device_model_object, "id"));
	device_model->discoverable = _discoverable_parse(device_model_object, "discoverable");
	device_model->id_template = json_object_get_string(json_object_object_get(device_model_object, "idTemplate"));

	json_object *sensors_object = NULL;
	if (json_object_object_get_ex(device_model_object, "sensors", &sensors_object) == false) {
		device_model->nr_sensors = 0;
		device_model->sensors = NULL;
		return;
	}

	int nr_sensors = json_object_array_length(sensors_object);
	device_model->sensors = calloc(nr_sensors, sizeof(struct sensor));
	if (device_model->sensors == NULL) {
		tube_log_error("[DISOCVER] calloc failed. size:%ld\n", sizeof(struct sensor));
		return;
	}

	int i;
	for (i=0; i<nr_sensors; i++) {
		json_object *sensor_object = NULL;
		sensor_object = json_object_array_get_idx(sensors_object, i);
		_sensor_parse(&device_model->sensors[i], sensor_object);
	}
	device_model->nr_sensors = nr_sensors;
}

static void _gw_model_cleanup(struct gateway_model* gw_model)
{
	if (!gw_model)
		return;

	int i;
	for (i=0; i<gw_model->nr_device_models; i++) {
		if (gw_model->device_models[i].sensors)
			free(gw_model->device_models[i].sensors);
	}

	if (gw_model->device_models)
		free(gw_model->device_models);

	tube_curl_paylaod_free(&gw_model->p);
}

static int _gw_model_parse(struct tube_curl_payload *p, struct gateway_model *gw_model)
{
	json_object *device_models_objects = NULL;
	if (json_object_object_get_ex(p->json, "deviceModels", &device_models_objects) == false) {
		tube_log_error("[DISCOVER] json_object_object_get_ex failed. key : deviceModels\n");
		return -1;
	}

	int nr_device_models = json_object_array_length(device_models_objects);
	gw_model->device_models = calloc(nr_device_models, sizeof(struct device_model));
	if (gw_model->device_models == NULL) {
		tube_log_error("[DISCOVER] calloc failed. size:%ld\n", sizeof(struct device_model));
		return -1;
	}
	gw_model->nr_device_models = nr_device_models;

	int i;
	for (i=0; i<nr_device_models; i++) {
		json_object *device_model_object = NULL;
		device_model_object = json_object_array_get_idx(device_models_objects, i);
		_device_model_parse(&gw_model->device_models[i], device_model_object);
	}

	return 0;
}

static int _gw_model_get(struct tube_http *t)
{
	char *url = tube_http_url_get(TUBE_HTTP_URL_GATEWAY_MODEL, t->gw_info.model);
	if (url == NULL) {
		tube_log_error("[DISCOVER] tube_http_url_get failed\n");
		return -1;
	}

	if (tube_curl_read(url, t->curl, &t->gw_model.p) < 0) {
		tube_log_error("[DISCOVER] tube_curl_read(TUBE_HTTP_URL_GATEWAY_MODEL) failed\n");
		tube_http_url_put(url);
		return -1;
	}

	_gw_model_parse(&t->gw_model.p,  &t->gw_model);

	tube_http_url_put(url);

	return 0;
}

static struct device_model* _device_model_get(struct gateway_model *gw_model, char *device_model_id)
{
	int i;
	for (i=0; i<gw_model->nr_device_models; i++) {
		if (strcasecmp(gw_model->device_models[i].id, device_model_id) == 0)
			return &gw_model->device_models[i];
	}

	return NULL;
}

static bool _is_device_duplicated(struct gateway_info *gw_info, char *device_id)
{
	int i;
	for (i=0; i<gw_info->nr_devices; i++) {
		if (strcasecmp(gw_info->devices[i], device_id) == 0) {
			return true;
		}
	}

	return false;
}

static bool _device_id_set(char* id_template, char *gw_id, unsigned int uid, char *device_id)
{
	if (id_template == NULL) {
		tube_log_error("[DISCOVER] id_template is NULL");
		return false;
	}

	if (strcasecmp(id_template, "{gatewayId}-{deviceAddress}") == 0) {
		sprintf(device_id, "%s-%d", gw_id, uid);
		return true;
	}
	else if (strcasecmp(id_template, "{gatewayId}") == 0) {
		sprintf(device_id, "%s", gw_id);
		return true;
	}
	else {
		char *postfix = id_template + strlen("{gatewayId}-");
		sprintf(device_id, "%s-%s", gw_id, postfix);
		return true;
	}

	return false;
}

static int _device_register(struct tdiscover* t, struct device_model *device_model, char *name, char* device_id)
{
	json_object *postfields_ojbect = json_object_new_object();
	json_object_object_add(postfields_ojbect, "reqId", json_object_new_string(device_id));
	json_object_object_add(postfields_ojbect, "name", json_object_new_string(name));
	json_object_object_add(postfields_ojbect, "model", json_object_new_string(device_model->id));

	char *url = tube_http_url_get(TUBE_HTTP_URL_DEVICE_REGISTER, t->gateway_id);
	if (url == NULL) {
		tube_log_error("[DISCOVER] tube_http_url_get failed\n");
		return -1;
	}

	struct tube_curl_payload p;
	if (tube_curl_post(url, t->curl, (void*)json_object_to_json_string(postfields_ojbect), &p) < 0) {
		tube_log_error("[DISCOVER] tube_curl_post(TUBE_HTTP_URL_DEVICE_REGISTER) failed\n");

		tube_http_url_put(url);
		json_object_put(postfields_ojbect);
		return -1;
	}

	tube_curl_paylaod_free(&p);
	tube_http_url_put(url);
	json_object_put(postfields_ojbect);

	return 0;
}

static struct sensor* _sensor_get(char *type, struct device_model *device_model)
{
	int i;
	for (i=0; i<device_model->nr_sensors; i++) {
		if (strcasecmp(device_model->sensors[i].type , type) == 0) {
			return &device_model->sensors[i];
		}
	}

	return NULL;
}

static bool _is_sensor_duplicated(struct gateway_info *gw_info, char *sensor_id)
{
	int i;
	for (i=0; i<gw_info->nr_sensors; i++) {
		if (strcasecmp(gw_info->sensors[i], sensor_id) == 0) {
			return true;
		}
	}

	return false;
}

static bool _sensor_id_set(char* id_template, struct sensor* sensor, char* gw_id, char* device_id, int uid, char* sensor_id)
{
	memset(sensor_id, 0, THINGPLUS_DISCOVER_MAX_ID);

	if (strcasecmp(id_template, "{type}-{gatewayId}-{sequence}") == 0) {
		sprintf(sensor_id, "%s-%s-%d", sensor->type, gw_id, uid);
		return true;
	}
	else if (strcasecmp(id_template, "{gatewayId}") == 0) {
		sprintf(sensor_id, "%s", gw_id); /* device_id is {gateway_id}-{deviceAddress} */
		return true;
	}
	else
		sprintf(sensor_id, "%s-%s-%d", device_id, sensor->type, uid);
		return true;
}

static int _sensor_register(struct tdiscover *t, char* name, int uid, char* device_id, struct sensor* sensor, char* sensor_id)
{
#if 0
	char* id_template = sensor_driver_id_template_get(t->curl, (char*)sensor->driver_name);
	if (!id_template) {
		tube_log_error("[DISCOVER] id_template get failed\n");
		return -1;
	}

	_sensor_id_set(id_template, sensor, t->gateway_id, device_id, uid, sensor_id);
#endif

	json_object *postfields_ojbect = json_object_new_object();
	json_object_object_add(postfields_ojbect, "reqId", json_object_new_string(sensor_id));
	json_object_object_add(postfields_ojbect, "category", json_object_new_string(sensor->category));
	json_object_object_add(postfields_ojbect, "type", json_object_new_string(sensor->type));
	json_object_object_add(postfields_ojbect, "model", json_object_new_string(sensor->model));
	json_object_object_add(postfields_ojbect, "driverName", json_object_new_string(sensor->driver_name));
	json_object_object_add(postfields_ojbect, "network", json_object_new_string(sensor->network));
	json_object_object_add(postfields_ojbect, "name", json_object_new_string(name));
	json_object_object_add(postfields_ojbect, "deviceId", json_object_new_string(device_id));

	char *url = tube_http_url_get(TUBE_HTTP_URL_SENSOR_REGISTER, t->gateway_id);
	if (url == NULL) {
		tube_log_error("[DISCOVER] tube_http_url_get failed\n");
		return -1;
	}

	struct tube_curl_payload p;
	if (tube_curl_post(url, t->curl, (void*)json_object_to_json_string(postfields_ojbect), &p) < 0) {
		tube_log_error("[DISCOVER] tube_curl_post(TUBE_HTTP_URL_SENSOR_REGISTER) failed\n");

		tube_http_url_put(url);
		json_object_put(postfields_ojbect);
		return -1;
	}

	tube_curl_paylaod_free(&p);
	tube_http_url_put(url);
	json_object_put(postfields_ojbect);

	return 0;
}

enum tube_http_error thingplus_discover_sensor_register(void* instance, char* name, int uid, char* type, char* device_id, char* sensor_id)
{
	struct tdiscover *t = (struct tdiscover *)instance;

	if (t == NULL) {
		tube_log_error("[DISCOVER] instance is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (name == NULL) {
		tube_log_error("[DISCOVER] name is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (type == NULL) {
		tube_log_error("[DISCOVER] type is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (device_id == NULL) {
		tube_log_error("[DISCOVER] device_id is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (sensor_id == NULL) {
		tube_log_error("[DISCOVER] sensor_id is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (t->gw_info.model == -1) {
		if (_gw_info_get(t) < 0) {
			tube_log_error("[DISCOVER] _gw_info_get failed\n");
			return TUBE_HTTP_ERROR_READ;
		}
	}

	if (!t->gw_info.discoverable) {
		tube_log_error("[DISCOVER] this gateway is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	if (t->gw_model.nr_device_models == 0) {
		if (_gw_model_get(t) < 0) {
			tube_log_error("[DISCOVER] _gw_model_get failed\n");
			return TUBE_HTTP_ERROR;
		}
	}

	struct device_model *dev_model = _device_model_get_by_device_id(t, device_id);
	if (dev_model == NULL) {
		tube_log_error("[DISCOVER] no device\n");
		return TUBE_HTTP_ERROR_READ;
	}

	if (!dev_model->discoverable) {
		tube_log_error("[DISCOVER] device is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	struct sensor *sensor = _sensor_get(type, dev_model);
	if (sensor == NULL) {
		return TUBE_HTTP_ERROR_READ;
	}

	char* id_template = sensor_driver_id_template_get(t->curl, (char*)sensor->driver_name);
	if (!id_template) {
		tube_log_error("[DISCOVER] id_template get failed\n");
		return -1;
	}
	_sensor_id_set(id_template, sensor, t->gateway_id, device_id, uid, sensor_id);

	if (_is_sensor_duplicated(&t->gw_info, sensor_id)) {
		tube_log_warn("[DISCOVER] sensor is already registered. name:%s\n", name);
		return TUBE_HTTP_ERROR_DUPLICATED;
	}

	if (_sensor_register(t, name, uid, device_id, sensor, sensor_id) < 0) {
		tube_log_error("[DISCOVER] _sensor_register failed. name:%s\n", name);
		return TUBE_HTTP_ERROR_POST;
	}

	return TUBE_HTTP_ERROR_NO_ERROR;
}

static struct device_model* _device_model_get_by_device_id(struct tube_http* t, char* device_id)
{
	struct device_model *dev_model = NULL;

	char *url = tube_http_url_get(TUBE_HTTP_URL_DEVICE_INFO, t->gateway_id, device_id);
	if (url == NULL) {
		tube_log_error("[DISCOVER] tube_http_url_get failed\n");
		goto err_tube_http_url_get;
	}

	struct tube_curl_payload p;
	if (tube_curl_read(url, t->curl, (void*)&p) < 0) {
		tube_log_error("[DISCOVER] tube_curl_read(TUBE_HTTP_URL_DEVICE_INFO) failed\n");
		goto err_tube_curl_read;
	}

	json_object *model_object = NULL;
	if (!json_object_object_get_ex(p.json, "model", &model_object)) {
		tube_log_error("[DISCOVER] json_object_object_get_ex failed. key : model\n");
		goto err_json_object_object_get_ex;
	}

	dev_model = _device_model_get(&t->gw_model, (char*)json_object_get_string(model_object));

err_json_object_object_get_ex:
	tube_curl_paylaod_free(&p);

err_tube_curl_read:
	tube_http_url_put(url);

err_tube_http_url_get:
	return dev_model;
}


enum tube_http_error thingplus_discover_device_register(void* instance , char* name, int uid, char* device_model_id, char* device_id)
{
	struct tdiscover *t = (struct tdiscover *)instance;

	if (t == NULL) {
		tube_log_error("tube_http is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (name == NULL) {
		tube_log_error("name is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (device_model_id == NULL) {
		tube_log_error("device_model_id is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (device_id == NULL) {
		tube_log_error("device_id is NULL\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (t->gw_info.model == -1) {
		if (_gw_info_get(t) < 0) {
			tube_log_error("[DISCOVER] _gw_info_get failed\n");
			return TUBE_HTTP_ERROR_READ;
		}
	}

	if (!t->gw_info.discoverable) {
		tube_log_error("[DISCOVER] this gateway is not discoverable\n");
		tube_log_error("[DISCOVER] gateway model number is %d\n",
			t->gw_info.model);
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	if (t->gw_model.nr_device_models == 0) {
		if (_gw_model_get(t) < 0) {
			tube_log_error("[DISCOVER] _gw_model_get failed\n");
			return TUBE_HTTP_ERROR;
		}
	}

	struct device_model *device_model = _device_model_get(&t->gw_model, device_model_id);
	if (device_model == NULL) {
		tube_log_error("[DISCOVER] _gw_model_get failed\n");
		return TUBE_HTTP_ERROR_DEVICE_MODEL_ID;
	}

	if (!device_model->discoverable) {
		tube_log_error("[DISCOVER] device model is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	memset(device_id, 0, THINGPLUS_DISCOVER_MAX_ID);
	if (!_device_id_set((char*)device_model->id_template, t->gateway_id, uid, device_id)) {
		tube_log_error("[DISCOVER] _device_id_set failed\n");
		tube_log_error("[DISCOVER] id_template is %s\n", device_model->id_template ? : "NULL");
		return TUBE_HTTP_ERROR_INVALID_TYPE;
	}

	if (_is_device_duplicated(&t->gw_info, device_id)) {
		tube_log_warn("[DISCOVER] device is already registered. name:%s\n", name);
		return TUBE_HTTP_ERROR_DUPLICATED;
	}

	if (_device_register(t, device_model, name, device_id) < 0) {
		tube_log_error("[DISCOVER] _device_register failed. name:%s\n", name);
		return TUBE_HTTP_ERROR_POST;
	}

	_gw_info_device_add(&t->gw_info, device_id);

	return TUBE_HTTP_ERROR_NO_ERROR;
}

void* thingplus_discover_init(char *gateway_id, char *apikey)
{
	tube_log_init(NULL);

	struct tdiscover *t = calloc(1, sizeof(struct tdiscover));
	if (t == NULL) {
		tube_log_error("calloc failed. size:%ld\n", 
			sizeof(struct tdiscover));
		return NULL;
	}

	if (gateway_id == NULL) {
		tube_log_error("gateway_id is NULL\n");

		free(t);
		return NULL;
	}

	if (apikey == NULL) {
		tube_log_error("apikey is NULL\n");

		free(t);
		return NULL;
	}

	t->gateway_id = strdup(gateway_id);
	t->apikey = strdup(apikey);
	t->curl = tube_curl_init(t->gateway_id, t->apikey);
	t->gw_info.model = -1;

	tube_log_info("gateway_id is :%s, apikey is :%s\n",
		t->gateway_id, t->apikey);

	return (void*)t;
}

void thingplus_discover_cleanup(void* instance)
{
	struct tdiscover *t = (struct tdiscover*)instance;

	if (t == NULL) {
		tube_log_warn("[DISCOVER] instance is NULL\n");
		return;
	}

	sensor_driver_cleanup();

	_gw_model_cleanup(&t->gw_model);
	_gw_info_cleanup(&t->gw_info);

	if (t->gateway_id)
		free(t->gateway_id);

	if (t->apikey)
		free(t->apikey);

	tube_curl_cleanup(t->curl);

	free(t);
}

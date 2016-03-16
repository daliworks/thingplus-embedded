#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <json/json.h>

#include "tube_curl.h"
#include "tube_http.h"
#include "tube_url.h"
#include "sensor_driver.h"

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

	struct tube_curl_payload p;
};

struct tube_http {
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
		fprintf(stderr, "[TUBE_HTTP] json_object_object_get_ex failed\n");
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
		fprintf(stderr, "[TUBE_HTTP] json_object_object_get_ex failed\n");
		return -1;
	}

	return json_object_get_int(model_object);
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

static void _gw_info_parse(struct tube_curl_payload *p, struct gateway_info* gw_info)
{
	gw_info->discoverable = _discoverable_parse(p->json, "autoCreateDiscoverable");
	gw_info->model = _gw_info_model_parse(p->json);
	gw_info->nr_devices = _gw_info_devices_parse(p->json, &gw_info->devices);
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

	tube_curl_paylaod_free(&gw_info->p);
}

static int _gw_info_read(struct tube_http *t)
{
	char *url = tube_http_url_get(TUBE_HTTP_URL_GATEWAY_INFO, t->gateway_id);
	if (url == NULL) {
		fprintf(stderr, "[TUBE_HTTP] tube_http_url_get failed\n");
		return -1;
	}

	tube_curl_read(url, t->curl, (void*)&t->gw_info.p);
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
		fprintf(stderr, "[TUBE_HTTP] calloc failed\n");
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
		fprintf(stderr, "[TUBE_HTTP] json_object_object_get_ex failed\n");
		return -1;
	}

	int nr_device_models = json_object_array_length(device_models_objects);
	gw_model->device_models = calloc(nr_device_models, sizeof(struct device_model));
	if (gw_model->device_models == NULL) {
		fprintf(stderr, "[TUBE_HTTP] calloc failed\n");
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

static int _gw_model_read(struct tube_http *t)
{
	char *url = tube_http_url_get(TUBE_HTTP_URL_GATEWAY_MODEL, t->gw_info.model);
	if (url == NULL) {
		fprintf(stderr, "[TUBE_HTTP] calloc failed\n");
		return -1;
	}

	tube_curl_read(url, t->curl, &t->gw_model.p);
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

static int _post_response_parse(struct tube_curl_payload *p)
{
	return atoi(p->payload) * -1;
}

static bool _device_id_set(char* id_template, char *gw_id, unsigned int uid, char *device_id)
{
	if (id_template == NULL) {
		fprintf(stderr, "[TUBE_HTTP] id_template is NULL");
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

static int _device_register(struct tube_http* t, struct device_model *device_model, char *name, unsigned int uid, char* device_id)
{
	memset(device_id, 0, TUBE_HTTP_MAX_ID);

	if (!_device_id_set((char*)device_model->id_template, t->gateway_id, uid, device_id)) {
		fprintf(stderr, "[TUBE_HTTP] _device_id_set failed\n");
		fprintf(stderr, "[TUBE_HTTP] id_template is %s\n", device_model->id_template ? : "NULL");
		return -1;
	}

	json_object *postfields_ojbect = json_object_new_object();
	json_object_object_add(postfields_ojbect, "reqId", json_object_new_string(device_id));
	json_object_object_add(postfields_ojbect, "name", json_object_new_string(name));
	json_object_object_add(postfields_ojbect, "model", json_object_new_string(device_model->id));

	char *url = tube_http_url_get(TUBE_HTTP_URL_DEVICE_REGISTER, t->gateway_id);
	if (url == NULL) {
		fprintf(stderr, "[TUBE_HTTP] tube_http_url_get failed\n");
		return -1;
	}

	struct tube_curl_payload p;
	if (tube_curl_post(url, t->curl, (void*)json_object_to_json_string(postfields_ojbect), &p) < 0) {
		fprintf(stderr, "[TUBE_HTTP] tube_curl_post failed\n");
		tube_http_url_put(url);
		return -1;
	}
	json_object_put(postfields_ojbect);

	int ret = _post_response_parse(&p);
	if (ret < 0) {
		fprintf(stderr, "[TUBE_HTTP] server response %d\n", ret);
		tube_http_url_put(url);
		return -1;
	}

	tube_http_url_put(url);
	tube_curl_paylaod_free(&p);

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

static bool _sensor_id_set(char* id_template, struct sensor* sensor, char* gw_id, char* device_id, int uid, char* sensor_id)
{
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

static int _sensor_register(struct tube_http *t, char* name, int uid, char* device_id, struct sensor* sensor, char* sensor_id)
{
	char* id_template = sensor_driver_id_template_get(t->curl, (char*)sensor->driver_name);
	if (!id_template) {
		fprintf(stderr, "[TUBE_HTTP] id_template get failed\n");
		return -1;
	}

	_sensor_id_set(id_template, sensor, t->gateway_id, device_id, uid, sensor_id);

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
		fprintf(stderr, "[TUBE_HTTP] tube_http_url_get failed\n");
		return -1;
	}

	struct tube_curl_payload p;
	if (tube_curl_post(url, t->curl, (void*)json_object_to_json_string(postfields_ojbect), &p) < 0) {
		tube_http_url_put(url);
		fprintf(stderr, "[TUBE_HTTP] tube_curl_post failed\n");
		return -1;
	}

	int ret = _post_response_parse(&p);
	if (ret < 0) {
		fprintf(stderr, "[TUBE_HTTP] server response %d\n", ret);
		tube_http_url_put(url);
		return -1;
	}

	tube_http_url_put(url);
	tube_curl_paylaod_free(&p);
	json_object_put(postfields_ojbect);

	return 0;
}

static struct device_model* _device_model_get_by_device_id(struct tube_http* t, char* device_id)
{
	struct device_model *dev_model = NULL;

	char *url = tube_http_url_get(TUBE_HTTP_URL_DEVICE_INFO, t->gateway_id, device_id);
	if (url == NULL) {
		fprintf(stderr, "[TUBE_HTTP] tube_http_url_get failed\n");
		goto err_tube_http_url_get;
	}

	struct tube_curl_payload p;
	if (tube_curl_read(url, t->curl, (void*)&p) < 0) {
		fprintf(stderr, "[TUBE_HTTP] tube_curl_read failed\n");
		goto err_tube_curl_read;
	}

	json_object *model_object = NULL;
	if (!json_object_object_get_ex(p.json, "model", &model_object)) {
		fprintf(stderr, "[TUBE_HTTP] json_object_object_get_ex failed\n");
		goto err_json_object_object_get_ex;
		return false;
	}

	dev_model = _device_model_get(&t->gw_model, (char*)json_object_get_string(model_object));

err_json_object_object_get_ex:
	tube_curl_paylaod_free(&p);

err_tube_curl_read:
	tube_http_url_put(url);

err_tube_http_url_get:

	return dev_model;
}

enum tube_http_error tube_http_sensor_register(void* tube_http, char* name, int uid, char* type, char* device_id, char* sensor_id)
{
	struct tube_http *t = (struct tube_http*)tube_http;
	if (t == NULL || name == NULL || type == NULL || device_id == NULL) {
		fprintf(stderr, "[TUBE_HTTP] invalid arguments\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (t->gw_info.model == -1) {
		if (_gw_info_read(t) < 0) {
			fprintf(stderr, "[TUBE_HTTP] _gw_info_read failed\n");
			return TUBE_HTTP_ERROR_READ;
		}
	}

	if (!t->gw_info.discoverable) {
		fprintf(stderr, "[TUBE_HTTP] this gateway is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	if (t->gw_model.nr_device_models == 0) {
		if (_gw_model_read(t) < 0) {
			fprintf(stderr, "[TUBE_HTTP] _gw_model_read failed\n");
			return TUBE_HTTP_ERROR;
		}
	}

	struct device_model *dev_model = _device_model_get_by_device_id(t, device_id);
	if (dev_model == NULL) {
		fprintf(stderr, "[TUBE_HTTP] no device\n");
		return TUBE_HTTP_ERROR_READ;
	}

	if (!dev_model->discoverable) {
		fprintf(stderr, "[TUBE_HTTP] device is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	struct sensor *sensor = _sensor_get(type, dev_model);
	if (sensor == NULL) {
		return TUBE_HTTP_ERROR_READ;
	}

	if (_sensor_register(t, name, uid, device_id, sensor, sensor_id) < 0) {
		fprintf(stderr, "[TUBE_HTTP] _sensor_register failed\n");
		return TUBE_HTTP_ERROR_POST;
	}

	return TUBE_HTTP_ERROR_NO_ERROR;
}

enum tube_http_error tube_http_device_register(void* tube_http, char* name, int uid, char* device_model_id, char* device_id)
{
	struct tube_http *t = (struct tube_http*)tube_http;

	if (t == NULL || name == NULL || device_model_id == NULL || device_id == NULL) {
		fprintf(stderr, "[TUBE_HTTP] invalid arguments\n");
		return TUBE_HTTP_ERROR_INVALID_ARGUMENT;
	}

	if (t->gw_info.model == -1) {
		if (_gw_info_read(t) < 0) {
			fprintf(stderr, "[TUBE_HTTP] _gw_info_read failed\n");
			return TUBE_HTTP_ERROR_READ;
		}
	}

	if (!t->gw_info.discoverable) {
		fprintf(stderr, "[TUBE_HTTP] this gateway is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	if (t->gw_model.nr_device_models == 0) {
		if (_gw_model_read(t) < 0) {
			fprintf(stderr, "[TUBE_HTTP] _gw_model_read failed\n");
			return TUBE_HTTP_ERROR;
		}
	}

	struct device_model *device_model = _device_model_get(&t->gw_model, device_model_id);
	if (device_model == NULL) {
		fprintf(stderr, "[TUBE_HTTP] _gw_model_read failed\n");
		return TUBE_HTTP_ERROR_DEVICE_MODEL_ID;
	}

	if (!device_model->discoverable) {
		fprintf(stderr, "[TUBE_HTTP] device model is not discoverable\n");
		return TUBE_HTTP_ERROR_NOT_DISCOVERABLE;
	}

	if (_device_register(t, device_model, name, uid, device_id) < 0) {
		fprintf(stderr, "[TUBE_HTTP] _device_register failed\n");
		return TUBE_HTTP_ERROR_POST;
	}

	return TUBE_HTTP_ERROR_NO_ERROR;
}

void* tube_http_init(char *gateway_id, char *apikey)
{
	struct tube_http *t = calloc(1, sizeof(struct tube_http));
	if (t == NULL) {
		return NULL;
	}

	if (gateway_id == NULL || apikey == NULL) {
		return NULL;
	}

	t->gateway_id = strdup(gateway_id);
	t->apikey = strdup(apikey);
	t->curl = tube_curl_init(t->gateway_id, t->apikey);
	t->gw_info.model = -1;


	return (void*)t;
}

void tube_http_cleanup(void* tube_http)
{
	struct tube_http *t = (struct tube_http*)tube_http;

	if (t == NULL) {
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

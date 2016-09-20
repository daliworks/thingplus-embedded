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

#include "thingplus.h"
#include "tcurl.h"
#include "sensor_driver.h"
#include "url.h"

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

	struct tcurl_payload p;
};

struct gateway_info {
	bool discoverable;
	int model;

	int nr_devices;
	char** devices;

	int nr_sensors;
	char** sensors;

	struct tcurl_payload p;
};

struct rest {
	char *gw_id;
	char *apikey;
	void* curl;

	struct gateway_info gw_info;
	struct gateway_model gw_model;
};

static bool _discoverable_parse(json_object *json, char *key)
{
	json_object *discoverable_object = NULL;
	if (!json_object_object_get_ex(json, key, &discoverable_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_ex failed. key : %s\n", key);
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
		fprintf(stdout, "[ERR] json_object_object_get_ex failed. key : model\n");
		return -1;
	}

	return json_object_get_int(model_object);
}

static void _gw_info_device_add(struct gateway_info *gw_info, char *device_id)
{
	int realloc_size = sizeof(char*) * (gw_info->nr_devices + 1);

	gw_info->devices = realloc(gw_info->devices, realloc_size);
	if (gw_info->devices == NULL) {
		fprintf(stdout, "[ERR] realloc failed. size:%d\n",
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

//static void _gw_info_parse(json_object *json, struct gateway_info* gw_info)
static void _gw_info_parse(struct tcurl_payload *p, struct gateway_info* gw_info)
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

	tcurl_payload_free(&gw_info->p);
}

static int _gw_info_get(struct rest *t)
{
	char *url = url_get(URL_INDEX_GATEWAY_INFO, t->gw_id);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url_get failed. gw_id:%s\n", t->gw_id);
		return -1;
	}

	if (tcurl_read(url, t->curl, (void*)&t->gw_info.p) < 0) {
		fprintf(stdout, "[ERR] tcurl_read(URL_INDEX_GATEWAY_INFO) failed. gw_id:%s\n", t->gw_id);

		url_put(url);
		return -1;
	}

	_gw_info_parse(&t->gw_info.p, &t->gw_info);

	url_put(url);
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
		fprintf(stdout, "[ERR] calloc failed. size:%ld\n", sizeof(struct sensor));
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

	tcurl_payload_free(&gw_model->p);
}

static int _gw_model_parse(struct tcurl_payload *p, struct gateway_model *gw_model)
{
	json_object *device_models_objects = NULL;
	if (json_object_object_get_ex(p->json, "deviceModels", &device_models_objects) == false) {
		fprintf(stdout, "[ERR] json_object_object_get_ex failed. key : deviceModels\n");
		return -1;
	}

	int nr_device_models = json_object_array_length(device_models_objects);
	gw_model->device_models = calloc(nr_device_models, sizeof(struct device_model));
	if (gw_model->device_models == NULL) {
		fprintf(stdout, "[ERR] calloc failed. size:%ld\n", sizeof(struct device_model));
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

static int _gw_model_get(struct rest *t)
{
	char *url = url_get(URL_INDEX_GATEWAY_MODEL, t->gw_info.model);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url_get failed\n");
		return -1;
	}

	if (tcurl_read(url, t->curl, &t->gw_model.p) < 0) {
		fprintf(stdout, "[ERR] tcurl_read(URL_INDEX_GATEWAY_MODEL) failed\n");
		url_put(url);
		return -1;
	}

	_gw_model_parse(&t->gw_model.p,  &t->gw_model);

	url_put(url);

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
		fprintf(stdout, "[ERR] id_template is NULL");
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

static int _device_register(struct rest* t, struct device_model *device_model, char *name, char* device_id)
{
	json_object *postfields_ojbect = json_object_new_object();
	json_object_object_add(postfields_ojbect, "reqId", json_object_new_string(device_id));
	json_object_object_add(postfields_ojbect, "name", json_object_new_string(name));
	json_object_object_add(postfields_ojbect, "model", json_object_new_string(device_model->id));

	char *url = url_get(URL_INDEX_DEVICE_REGISTER, t->gw_id);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url_get failed\n");
		return -1;
	}

	struct tcurl_payload p;
	if (tcurl_post(url, t->curl, (void*)json_object_to_json_string(postfields_ojbect), &p) < 0) {
		fprintf(stdout, "[ERR] tcurl_post(URL_INDEX_DEVICE_REGISTER) failed\n");

		url_put(url);
		json_object_put(postfields_ojbect);
		return -1;
	}

	tcurl_payload_free(&p);
	url_put(url);
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
	memset(sensor_id, 0, THINGPLUS_ID_LENGTH);

	if (strcasecmp(id_template, "{type}-{gatewayId}-{sequence}") == 0) {
		sprintf(sensor_id, "%s-%s-%d", sensor->type, gw_id, uid);
		return true;
	}
	else if (strcasecmp(id_template, "{gatewayId}") == 0) {
		sprintf(sensor_id, "%s", gw_id); /* device_id is {gw_id}-{deviceAddress} */
		return true;
	}
	else
		sprintf(sensor_id, "%s-%s-%d", device_id, sensor->type, uid);
		return true;
}

static int _sensor_register(struct rest *t, char* name, int uid, char* device_id, struct sensor* sensor, char* sensor_id)
{
#if 0
	char* id_template = sensor_driver_id_template_get(t->curl, (char*)sensor->driver_name);
	if (!id_template) {
		fprintf(stdout, "[ERR] id_template get failed\n");
		return -1;
	}

	_sensor_id_set(id_template, sensor, t->gw_id, device_id, uid, sensor_id);
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

	char *url = url_get(URL_INDEX_SENSOR_REGISTER, t->gw_id);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url_get failed\n");
		return -1;
	}

	struct tcurl_payload p;
	if (tcurl_post(url, t->curl, (void*)json_object_to_json_string(postfields_ojbect), &p) < 0) {
		fprintf(stdout, "[ERR] tcurl_post(URL_INDEX_SENSOR_REGISTER) failed\n");

		url_put(url);
		json_object_put(postfields_ojbect);
		return -1;
	}

	tcurl_payload_free(&p);
	url_put(url);
	json_object_put(postfields_ojbect);

	return 0;
}

static struct device_model* _device_model_get_by_device_id(struct rest* t, char* device_id)
{
	struct device_model *dev_model = NULL;

	char *url = url_get(URL_INDEX_DEVICE_INFO, t->gw_id, device_id);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url_get failed\n");
		goto err_url_get;
	}

	struct tcurl_payload p;
	if (tcurl_read(url, t->curl, (void*)&p) < 0) {
		fprintf(stdout, "[ERR] tcurl_read(URL_INDEX_DEVICE_INFO) failed\n");
		goto err_tcurl_read;
	}

	json_object *model_object = NULL;
	if (!json_object_object_get_ex(p.json, "model", &model_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_ex failed. key : model\n");
		goto err_json_object_object_get_ex;
	}

	dev_model = _device_model_get(&t->gw_model, (char*)json_object_get_string(model_object));

err_json_object_object_get_ex:
	tcurl_payload_free(&p);

err_tcurl_read:
	url_put(url);

err_url_get:
	return dev_model;
}

int rest_gatewayinfo(void *instance, struct thingplus_gatewayinfo *gw_info)
{
	return 1;
}


static bool _deviceinfo_name_parse(json_object *json, char *name)
{
	json_object *name_object = NULL;
	if (!json_object_object_get_ex(json, "name", &name_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : name\n");
		return false;
	}

	strncpy(name, json_object_get_string(name_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static bool _deviceinfo_model_parse(json_object *json, char* model)
{
	json_object *model_object = NULL;
	if (!json_object_object_get_ex(json, "model", &model_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : model\n");
		return false;
	}

	strncpy(model, json_object_get_string(model_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static bool _deviceinfo_owner_parse(json_object *json, char *owner)
{
	json_object *owner_object = NULL;
	if (!json_object_object_get_ex(json, "owner", &owner_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : owner\n");
		return false;
	}

	strncpy(owner, json_object_get_string(owner_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static time_t _deviceinfo_mtime_parse(json_object *json) 
{
	json_object *mtime_object = NULL;
	if (!json_object_object_get_ex(json, "mtime", &mtime_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : mtime\n");
		return false;
	}
	return json_object_get_int64(mtime_object);
}

static time_t _deviceinfo_ctime_parse(json_object* json)
{
	json_object *ctime_object = NULL;
	if (!json_object_object_get_ex(json, "ctime", &ctime_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : ctime\n");
		return false;
	}
	return json_object_get_int64(ctime_object);
}

static bool _deviceinfo_id_parse(json_object* json, char *id)
{
	json_object *dev_id_object = NULL;
	if (!json_object_object_get_ex(json, "id", &dev_id_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : id\n");
		return false;
	}

	strncpy(id, json_object_get_string(dev_id_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static void _deviceinfo_parse(json_object *json, struct thingplus_device *dev_info)
{
	memset(dev_info, 0, sizeof(struct thingplus_device));

	_deviceinfo_name_parse(json, dev_info->name);
	_deviceinfo_model_parse(json, dev_info->model);
	_deviceinfo_owner_parse(json, dev_info->owner);
	dev_info->mtime = _deviceinfo_mtime_parse(json);
	dev_info->ctime = _deviceinfo_ctime_parse(json);
	_deviceinfo_id_parse(json, dev_info->id);
}

static bool _sensorinfo_network_parse(json_object *json, char *network)
{
	json_object *network_object = NULL;
	if (!json_object_object_get_ex(json, "network", &network_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : network\n");
		return false;
	}

	strncpy(network, json_object_get_string(network_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static bool _sensorinfo_driver_name_parse(json_object *json, char *driver_name)
{
	json_object *driver_name_object = NULL;
	if (!json_object_object_get_ex(json, "driverName", &driver_name_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : driver_name\n");
		return false;
	}

	strncpy(driver_name, json_object_get_string(driver_name_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static bool _sensorinfo_type_parse(json_object *json, char *type)
{
	json_object *type_object = NULL;
	if (!json_object_object_get_ex(json, "type", &type_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : type\n");
		return false;
	}

	strncpy(type, json_object_get_string(type_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static bool _sensorinfo_category_parse(json_object *json, char *category)
{
	json_object *category_object = NULL;
	if (!json_object_object_get_ex(json, "category", &category_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : category\n");
		return false;
	}

	strncpy(category, json_object_get_string(category_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static bool _sensorinfo_device_id_parse(json_object *json, char *device_id)
{
	json_object *device_id_object = NULL;
	if (!json_object_object_get_ex(json, "device_id", &device_id_object)) {
		fprintf(stdout, "[ERR] json_object_object_get_exa failed. key : device_id\n");
		return false;
	}

	strncpy(device_id, json_object_get_string(device_id_object), THINGPLUS_NAME_LENGTH);

	return true;
}

static void _sensorinfo_parse(json_object *json, struct thingplus_sensor *sensor_info)
{
	memset(sensor_info, 0, sizeof(struct thingplus_device));

	_deviceinfo_name_parse(json, sensor_info->name);
	_deviceinfo_model_parse(json, sensor_info->model);
	_deviceinfo_owner_parse(json, sensor_info->owner);
	sensor_info->mtime = _deviceinfo_mtime_parse(json);
	sensor_info->ctime = _deviceinfo_ctime_parse(json);
	_deviceinfo_id_parse(json, sensor_info->id);
	_sensorinfo_network_parse(json, sensor_info->network);
	_sensorinfo_driver_name_parse(json, sensor_info->driver_name);
	_sensorinfo_type_parse(json, sensor_info->type);
	_sensorinfo_category_parse(json, sensor_info->category);
	_sensorinfo_device_id_parse(json, sensor_info->device_id);
}

int rest_deviceinfo(void *instance, char *id, struct thingplus_device *dev_info)
{
	int ret = -1;
	struct rest *r =  (struct rest *)instance;

	char *url = url_get(URL_INDEX_DEVICE_INFO, r->gw_id, id);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url get failed\n");
		return ret;
	}

	struct tcurl_payload p;
	if (tcurl_read(url, r->curl, (void*)&p) < 0) {
		fprintf(stdout, "[ERR] tcurl_read(URL_INDEX_DEVICE_INFO) failed\n");
		goto err_tcurl_read;
	}

	ret = 0;
	_deviceinfo_parse(p.json, dev_info);

err_tcurl_read:
	url_put(url);

	return ret;
}

int rest_sensorinfo(void *instance, char *id, struct thingplus_sensor *sensor_info)
{
	int ret = -1;
	struct rest *r =  (struct rest *)instance;

	char *url = url_get(URL_INDEX_SENSOR_INFO, r->gw_id, id);
	if (url == NULL) {
		fprintf(stdout, "[ERR] url get failed\n");
		return ret;
	}

	struct tcurl_payload p;
	if (tcurl_read(url, r->curl, (void*)&p) < 0) {
		fprintf(stdout, "[ERR] tcurl_read(URL_INDEX_DEVICE_INFO) failed\n");
		goto err_tcurl_read;
	}

	ret = 0;
	_sensorinfo_parse(p.json, sensor_info);

err_tcurl_read:
	url_put(url);

	return ret;
}

int rest_sensor_register(void* instance, char* name, int uid, char* type, char* device_id, char* sensor_id)
{
	struct rest *t = (struct rest*) instance;

	if (t == NULL) {
		fprintf(stdout, "[ERR] instance is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (name == NULL) {
		fprintf(stdout, "[ERR] name is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (type == NULL) {
		fprintf(stdout, "[ERR] type is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (device_id == NULL) {
		fprintf(stdout, "[ERR] device_id is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (sensor_id == NULL) {
		fprintf(stdout, "[ERR] sensor_id is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (t->gw_info.model == -1) {
		if (_gw_info_get(t) < 0) {
			fprintf(stdout, "[ERR] _gw_info_get failed\n");
			return THINGPLUS_ERR_CURL_READ;
		}
	}

	if (!t->gw_info.discoverable) {
		fprintf(stdout, "[ERR] this gateway is not discoverable\n");
		return THINGPLUS_ERR_NOT_DISOVERABLE;
	}

	if (t->gw_model.nr_device_models == 0) {
		if (_gw_model_get(t) < 0) {
			fprintf(stdout, "[ERR] _gw_model_get failed\n");
			return THINGPLUS_ERR_UNKNOWN;
		}
	}

	struct device_model *dev_model = _device_model_get_by_device_id(t, device_id);
	if (dev_model == NULL) {
		fprintf(stdout, "[ERR] no device\n");
		return THINGPLUS_ERR_CURL_READ;
	}

	if (!dev_model->discoverable) {
		fprintf(stdout, "[ERR] device is not discoverable\n");
		return THINGPLUS_ERR_NOT_DISOVERABLE;
	}

	struct sensor *sensor = _sensor_get(type, dev_model);
	if (sensor == NULL) {
		return THINGPLUS_ERR_CURL_READ;
	}

	char* id_template = sensor_driver_id_template_get(t->curl, (char*)sensor->driver_name);
	if (!id_template) {
		fprintf(stdout, "[ERR] id_template get failed\n");
		return -1;
	}
	_sensor_id_set(id_template, sensor, t->gw_id, device_id, uid, sensor_id);

	if (_is_sensor_duplicated(&t->gw_info, sensor_id)) {
		fprintf(stdout, "[ERR] sensor is already registered. name:%s\n", name);
		return THINGPLUS_ERR_DUPLICATED;
	}

	if (_sensor_register(t, name, uid, device_id, sensor, sensor_id) < 0) {
		fprintf(stdout, "[ERR] _sensor_register failed. name:%s\n", name);
		return THINGPLUS_ERR_CURL_POST;
	}

	return THINGPLUS_ERR_SUCCESS;
}

enum thingplus_error rest_device_register(void* instance, char* name, int uid, char* device_model_id, char* device_id)
{
	struct rest *t = (struct rest*) instance;

	if (t == NULL) {
		fprintf(stdout, "instance is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (name == NULL) {
		fprintf(stdout, "name is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (device_model_id == NULL) {
		fprintf(stdout, "device_model_id is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (device_id == NULL) {
		fprintf(stdout, "device_id is NULL\n");
		return THINGPLUS_ERR_SUCCESS;
	}

	if (t->gw_info.model == -1) {
		if (_gw_info_get(t) < 0) {
			fprintf(stdout, "[ERR] _gw_info_get failed\n");
			return THINGPLUS_ERR_CURL_READ;
		}
	}

	if (!t->gw_info.discoverable) {
		fprintf(stdout, "[ERR] this gateway is not discoverable\n");
		fprintf(stdout, "[ERR] gateway model number is %d\n",
			t->gw_info.model);
		return THINGPLUS_ERR_NOT_DISOVERABLE;
	}

	if (t->gw_model.nr_device_models == 0) {
		if (_gw_model_get(t) < 0) {
			fprintf(stdout, "[ERR] _gw_model_get failed\n");
			return THINGPLUS_ERR_UNKNOWN;
		}
	}

	struct device_model *device_model = _device_model_get(&t->gw_model, device_model_id);
	if (device_model == NULL) {
		fprintf(stdout, "[ERR] _device_model_get failed\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (!device_model->discoverable) {
		fprintf(stdout, "[ERR] device model is not discoverable\n");
		return THINGPLUS_ERR_NOT_DISOVERABLE;
	}

	memset(device_id, 0, THINGPLUS_ID_LENGTH);
	if (!_device_id_set((char*)device_model->id_template, t->gw_id, uid, device_id)) {
		fprintf(stdout, "[ERR] _device_id_set failed\n");
		fprintf(stdout, "[ERR] id_template is %s\n", device_model->id_template ? : "NULL");
		return THINGPLUS_ERR_INVALID_TYPE;
	}

	if (_is_device_duplicated(&t->gw_info, device_id)) {
		fprintf(stdout, "[ERR] device is already registered. name:%s\n", name);
		return THINGPLUS_ERR_DUPLICATED;
	}

	if (_device_register(t, device_model, name, device_id) < 0) {
		fprintf(stdout, "[ERR] _device_register failed. name:%s\n", name);
		return THINGPLUS_ERR_CURL_POST;
	}

	_gw_info_device_add(&t->gw_info, device_id);

	return THINGPLUS_ERR_SUCCESS;
}

void* rest_init(char *gw_id, char *apikey, char *rest_url)
{
	if (gw_id == NULL) {
		fprintf(stdout, "gw_id is NULL\n");
		return NULL;
	}

	if (apikey == NULL) {
		fprintf(stdout, "apikey is NULL\n");
		return NULL;
	}

	struct rest *t = calloc(1, sizeof(struct rest));
	if (t == NULL) {
		fprintf(stdout, "calloc failed. size:%ld\n", 
			sizeof(struct rest));
		return NULL;
	}

	t->curl = tcurl_init(gw_id, apikey);
	if (!t->curl) {
		fprintf(stdout, "[ERR] tcurl_init failed\n");
		goto err_tcurl_init;
	}

	t->gw_info.model = -1;
	t->gw_id = strdup(gw_id);
	t->apikey = strdup(apikey);

	fprintf(stdout, "gw_id is :%s, apikey is :%s\n", t->gw_id, t->apikey);

	return (void*)t;

err_tcurl_init:
	free(t);

	return NULL;
}

void rest_cleanup(void* instance)
{
	struct rest *t = (struct rest *)instance;

	if (t == NULL) {
		fprintf(stdout, "instance is NULL\n");
		return;
	}

	sensor_driver_cleanup();
	_gw_model_cleanup(&t->gw_model);
	_gw_info_cleanup(&t->gw_info);

	if (t->gw_id)
		free(t->gw_id);

	if (t->apikey)
		free(t->apikey);

	tcurl_cleanup(t->curl);

	free(t);
}

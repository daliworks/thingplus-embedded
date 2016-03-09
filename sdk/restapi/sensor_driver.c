#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <json/json.h>

#include "tube_curl.h"
#include "tube_http.h"
#include "tube_url.h"

struct sensor_driver {
	char* driver_name;
	char* id_template;
};

int nr_list = 0;
static struct sensor_driver *list = NULL;

static struct sensor_driver* _sensor_drivers_get(void* curl, char *driver_name)
{
	char *url = tube_http_url_get(TUBE_HTTP_URL_SENSOR_DRIVERS, driver_name);
	if (url == NULL) {
		fprintf(stderr, "[SENSOR_DRIVER] tube_http_url_get failed. driver_name:%s\n", driver_name);
		return NULL;
	}

	struct tube_curl_payload p;
	if (tube_curl_read(url, curl, &p) < 0) {
		fprintf(stderr, "[SENSOR_DRIVER] tube_curl_read_ failed\n");
		tube_http_url_put(url);
		return NULL;
	}

	json_object *sensor_driver_object = json_object_array_get_idx(p.json, 0);
	json_object *id_template_object = NULL;
	if (!json_object_object_get_ex(sensor_driver_object, "idTemplate", &id_template_object)) {
		fprintf(stderr, "[SENSOR_DRIVER] json_object_object_get_ex failed\n");
		tube_http_url_put(url);
		tube_curl_paylaod_free(&p);
		return NULL;
	}

	list = realloc(list, sizeof(struct sensor_driver) * (nr_list +1));
	if (list == NULL) {
		fprintf(stderr, "[SENSOR_DRIVER] realloc failed");
		tube_http_url_put(url);
		tube_curl_paylaod_free(&p);
		return NULL;
	}

	list[nr_list].driver_name = strdup(driver_name);
	list[nr_list].id_template = strdup(json_object_get_string(id_template_object));

	tube_http_url_put(url);
	tube_curl_paylaod_free(&p);

	return &list[nr_list++];
}

static struct sensor_driver* _sensor_driver_search(char* driver_name)
{
	int i;
	for (i=0; i<nr_list; i++) {
		if (strcasecmp(list[i].driver_name, driver_name) == 0)
			return &list[i];
	}

	return NULL;
}

char* sensor_driver_id_template_get(void* tube_curl, char* driver_name)
{
	struct sensor_driver* sd = _sensor_driver_search(driver_name);
	if (sd) {
		return sd->id_template;
	}

	sd = _sensor_drivers_get(tube_curl, driver_name);
	if (sd == NULL) {
		fprintf(stderr, "[SENSOR_DRIVER] _sensor_drivers_get failed\n");
		return NULL;
	}

	return sd->id_template;
}

void sensor_driver_cleanup(void)
{
	if (!list)
		return;

	int i;
	for (i=0; i<nr_list; i++)
	{
		if (list[i].driver_name)
			free(list[i].driver_name);
		if (list[i].id_template)
			free(list[i].id_template);
	}

	free(list);
}

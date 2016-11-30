#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "tcurl.h"
#include "url.h"

struct sensor_driver {
	char* driver_name;
	char* id_template;
};

int nr_list = 0;
static struct sensor_driver *list = NULL;

static struct sensor_driver* _sensor_drivers_get(void* curl, char *driver_name)
{
	char *url = url_get(URL_INDEX_SENSOR_DRIVERS, driver_name);
	if (url == NULL) {
		fprintf(stdout, "[SENSOR_DRIVER] url_get failed. driver_name:%s\n", driver_name);
		goto err_url_get;
	}

	struct tcurl_payload p;
	if (tcurl_read(url, curl, &p) < 0) {
		fprintf(stdout, "[SENSOR_DRIVER] tube_curl_read_ failed\n");
		goto err_tcurl_read;
	}

	json_object *sensor_driver_object = p.json;
	json_object *id_template_object = NULL;
	if (!json_object_object_get_ex(p.json, "idTemplate", &id_template_object)) {
		fprintf(stdout, "[SENSOR_DRIVER] json_object_object_get_ex failed\n");
		goto err_json_object_object_get_ex;
	}

	int realloc_size = sizeof(struct sensor_driver) * (nr_list +1);
	list = realloc(list, realloc_size);
	if (list == NULL) {
		fprintf(stdout, "[SENSOR_DRIVER] realloc failed. realloc size:%d",
			realloc_size);

		list = NULL;
		nr_list = 0;
		goto err_realloc;
	}

	list[nr_list].driver_name = strdup(driver_name);
	list[nr_list].id_template = strdup(json_object_get_string(id_template_object));

	tcurl_payload_free(&p);
	url_put(url);

	return &list[nr_list++];

err_realloc:
err_json_object_object_get_ex:
err_tcurl_read:
	tcurl_payload_free(&p);
	url_put(url);
err_url_get:
	return NULL;
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
		fprintf(stdout, "[SENSOR_DRIVER] _sensor_drivers_get failed. driver_name:%s\n",
			driver_name);
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

	list = NULL;
	nr_list = 0;
}

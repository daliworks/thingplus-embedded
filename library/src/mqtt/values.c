#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thingplus_types.h>

#include "values.h"

struct values{
	struct list_head sensors_head;
};

static struct values_sensor* _sensor_get(struct values* v, char *id)
{
	struct values_sensor *values_s;

	list_for_each_entry(values_s, &v->sensors_head, sensor) {
		if (strcmp(values_s->id, id) == 0) {
			return values_s;
		}
	}

	values_s = calloc(1, sizeof(struct values_sensor));
	if (values_s == NULL) {
		fprintf(stdout, "calloc failed\n");
		return NULL;
	}

	values_s->id = id;

	list_add_tail(&values_s->sensor, &v->sensors_head);
	return values_s;
}

static void _sensor_value_push(struct values_sensor *values_s, struct thingplus_value *v)
{
	if (values_s->nr_allocated == values_s->nr_value) {
		values_s->nr_allocated += 10;
		values_s->value = realloc(values_s->value, 
				sizeof(struct thingplus_value) * values_s->nr_allocated);
	}

	memcpy(&values_s->value[values_s->nr_value++], v, sizeof(struct thingplus_value));
}

void* values_init(int nr_value, struct thingplus_value *v)
{
	struct values *values = calloc(1, sizeof(values));
	if (values == NULL) {
		fprintf(stdout, "calloc failed\n");
		return NULL;
	}

	INIT_LIST_HEAD(&values->sensors_head);

	int i;
	struct values_sensor *values_s;

	for (i=0; i<nr_value; i++) {
		if (!v[i].id)
			continue;

		values_s = _sensor_get(values, v[i].id);
		if (values_s == NULL) {
			goto err_svl_sensor_get;
		}
		_sensor_value_push(values_s, &v[i]);
	}

	return values;

err_svl_sensor_get:
	values_cleanup(values);
	return NULL;
}

void values_cleanup(void *_v)
{
	if (_v == NULL) {
		fprintf(stdout, "values instance is NULL\n");
		return;
	}

	struct values *values = (struct values*)_v;
	struct values_sensor *values_s;
	values_for_each_sensor(values_s, values) {
		if (!values_s)
			continue;

		if(values_s->value)
			free(values_s->value);
		free(values_s);
	}

	free(values);
}

#ifndef _VALUES_H_
#define _VALUES_H_

#include <thingplus_types.h>
#include "list.h"

struct values_sensor {
	char *id;
	int nr_allocated;
	int nr_value;
	struct thingplus_value *value;

	struct list_head sensor;
};

#define values_for_each_sensor(values_sensor, s) \
	list_for_each_entry(values_sensor, (struct list_head*)s, sensor)

void* values_init(int nr_value, struct thingplus_value *v);
void values_cleanup(void *_s);

#endif //#ifndef _VALUES_H_

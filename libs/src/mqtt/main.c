#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tube_mqtt.h"

static void _connect(void* arg, enum tube_mqtt_error result) 
{
	printf("%d\n", result);
}

void _mqtt_values_send(void* m)
{
	int i;
	struct tube_thing_values vs[3];
	int nr = 20;
	vs[0].id = "humidity-000011112222-0";
	vs[0].nr_value = nr;
	vs[0].value = calloc(nr, sizeof(struct tube_thing_value));
	for(i=0; i<nr; i++) {
		vs[0].value[i].value = "13";
		vs[0].value[i].time_ms = (time(NULL) - 60*(nr-i)) * 1000;
	}

	vs[1].id = "temperature-000011112222-0";
	vs[1].nr_value = nr;
	vs[1].value = calloc(nr, sizeof(struct tube_thing_value));
	for(i=0; i<nr; i++) {
		vs[1].value[i].value = "26.4";
		vs[1].value[i].time_ms = (time(NULL) - 60*(nr-i)) * 1000;
	}

	tube_mqtt_values_publish(m, 2, vs);
}

void _mqtt_status_send(void* m)
{
	struct tube_thing_status things[3];
	things[0].id = "onoff-000011112222-0";
	things[0].status = TUBE_STATUS_ON;
	things[0].valid_duration_sec = 60 * 10;

	things[1].id = "humidity-000011112222-0";
	things[1].status = TUBE_STATUS_ON;
	things[1].valid_duration_sec = 60 * 10;

	things[2].id = "000011112222";
	things[2].status = TUBE_STATUS_ON;
	things[2].valid_duration_sec = 60 * 10;

	tube_mqtt_status_publish(m, 3, things);
}

int main(void)
{
	struct tube_mqtt_callback mqtt_cb = {
		.connect = _connect,
	};
	void *m = tube_mqtt_connect(
			"000011112222", 
			"nh7-y1lw5c-aocYoaEXVTbwoXqU=", 
			"./cert/ca-cert.pem", 
			60,
			&mqtt_cb,
			NULL,
			NULL);

	sleep(1);
	_mqtt_status_send(m);

	sleep(1);
	_mqtt_values_send(m);
	sleep(3);

	//tube_mqtt_disconnect(m);
	while(1);

	return;

	/*
	struct tube_thing_value v;
	v.time_ms = time(NULL) * 1000;
	v.value = "26.4";

	tube_mqtt_single_value_publish(m, "humidity-000011112222-0", &v);
	*/

	struct tube_thing_value vv[10];
	int i;
	for (i=0; i<10; i++) {
		vv[i].time_ms = (time(NULL) - 60*(10-i)) * 1000;
		vv[i].value = "10";
	}


	struct tube_thing_values v;
	v.id = "humidity-000011112222-0";
	v.nr_value = 10;
	v.value = vv;

	tube_mqtt_values_publish(m, 1, &v);


	while(1);
	tube_mqtt_disconnect(m);
	return 0;
}

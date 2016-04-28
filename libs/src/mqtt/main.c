#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "tube_mqtt.h"

struct thing {
	void *thingplus;
};

static void _connect(void* arg, enum thingplus_error result) 
{
	printf("%d\n", result);
}

void _values_send(void* thingplus)
{
	int i;
	struct thingplus_values vs[3];
	int nr = 20;
	vs[0].id = "humidity-000011112222-0";
	vs[0].nr_value = nr;
	vs[0].value = calloc(nr, sizeof(struct thingplus_value));
	for(i=0; i<nr; i++) {
		vs[0].value[i].value = "13";
		vs[0].value[i].time_ms = (time(NULL) - 60*(nr-i)) * 1000;
	}

	vs[1].id = "temperature-000011112222-0";
	vs[1].nr_value = nr;
	vs[1].value = calloc(nr, sizeof(struct thingplus_value));
	for(i=0; i<nr; i++) {
		vs[1].value[i].value = "26.4";
		vs[1].value[i].time_ms = (time(NULL) - 60*(nr-i)) * 1000;
	}

	thingplus_values_publish(thingplus, 2, vs);
}

void _status_publish(void* thingplus)
{
	struct thingplus_status things[3];
	things[0].id = "onoff-000011112222-0";
	things[0].status = THINGPLUS_STATUS_ON;
	things[0].valid_duration_sec = 1 * 60;

	things[1].id = "humidity-000011112222-0";
	things[1].status = THINGPLUS_STATUS_ON;
	things[1].valid_duration_sec = 1 * 60;

	things[2].id = "000011112222";
	things[2].status = THINGPLUS_STATUS_ON;
	things[2].valid_duration_sec = 1 * 60;

	thingplus_status_publish(thingplus, 3, things);
}

static void _connected(void *cb_arg, thingplus_error_e err)
{
	struct thing *t = (struct thing*)cb_arg;

	printf("connected. err:%d\n", err);
	_status_publish(t->thingplus);
	_values_send(t->thingplus);
}

static void _disconnected(void *cb_arg, bool force)
{
	printf("disconnected\n");
}

static bool _actuating(void *cb_arg, const char *id, const char *cmd, const char *options, char **message)
{
	*message = "success";
	return true;
}

int main(void)
{
	struct thing *t = calloc(1, sizeof(struct thing));
	if (t == NULL) {
		printf("calloc failed\n");
		return -1;
	}

	char *gw_id = "000011112222";
	char *apikey = "nh7-y1lw5c-aocYoaEXVTbwoXqU=";
	char *ca_file = "./cert/ca-cert.pem";

	struct thingplus_callback cb = {
		.connected = _connected,
		.disconnected = _disconnected,
		.actuating = _actuating,
		//.timesync = NULL,
	};

	t->thingplus = thingplus_init(gw_id, apikey, ca_file, 60, &cb, t);
	if (t->thingplus == NULL) {
		printf("thingplus_init failed\n");
		return 0;
	}

	int ret = thingplus_connect(t->thingplus);
	if (ret != 0) {

		printf("thingplus_connect failed\n");
		return 0;
	}

	while(1) {
		sleep (1);
		thingplus_mqtt_loop(t->thingplus);
	}

	return 0;


	thingplus_disconnect(t->thingplus);

err_thingplus_connect:
	thingplus_cleanup(t->thingplus);

	free(t);

	return -1;
#if 0
	struct thingplus_callback mqtt_cb = {
		.connect = _connect,
	};
	void *m = tube_mqtt_connect(
			"000011112222", 
			"nh7-y1lw5c-aocYoaEXVTbwoXqU=", 
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

	return 0;

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
#endif
}

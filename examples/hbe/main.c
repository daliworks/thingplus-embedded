#include <stdio.h>
#include <stdlib.h>
#include <tdiscover.h>
#include <thingplus.h>
#include <signal.h>
#include <time.h>

#include "config.h"
#include "pir.h"
#include "sht20.h"

struct sensor {
	char *name;
	int uid;
	char *type;

	char sensor_id[TDISCOVER_MAX_ID];
	struct {
		void* (*init)(void *, void (*ops)(void*, char*));
		int (*get)(void*);
		int (*set)(void*, char *, char *);
	} ops;
	void *s;
};

struct device {
	char *name;
	int uid;
	char *device_model_id;
	char device_id[TDISCOVER_MAX_ID];
	int nr_sensor;
	struct sensor *sensors;
};

struct thing {
	int nr_device;
	struct device *devices;
};

static struct sensor hbe_sensors[] = {
	{.name = "PIR", .uid = 0, .type = "motion", .ops.init = pir_init, .ops.get = pir_get},
	{.name = "TEMPERATURE", .uid = 1, .type = "temperature", .ops.init = sht20_init, .ops.get = sht20_temperature_get},
	{.name = "HUMIDITY", .uid = 2, .type = "humidity", .ops.get = sht20_humidity_get},
};

static struct device hbe_device = {
	.name = "HBE",
	.uid = 1,
	.device_model_id = "jsonrpcFullV1.0",
	.nr_sensor = (sizeof(hbe_sensors)/sizeof(hbe_sensors[0])),
	.sensors = hbe_sensors,
};

static struct thing hbe = {
	.nr_device = 1,
	.devices = &hbe_device,
};

struct hbe {
	void *t;
	void *tdiscover;
	bool thingplus_connected;
	timer_t timer_id;
	int report_interval;
};

static unsigned long long _now_ms(void)
{
        struct timeval tv;
        unsigned long long now;

        gettimeofday(&tv, NULL);
        now = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

        return now;
}

static void _value_status_publish(union sigval sigval)
{
	struct hbe *h = sigval.sival_ptr;
	printf("%s\n", __func__);
	struct thingplus_status thing_status[4] = {
		{.id = gw_id, .status = THINGPLUS_STATUS_ON, .valid_duration_sec = h->report_interval * 2},
		{.id = hbe_sensors[0].sensor_id, .status = THINGPLUS_STATUS_ON, .valid_duration_sec = h->report_interval * 2},
		{.id = hbe_sensors[1].sensor_id, .status = THINGPLUS_STATUS_ON, .valid_duration_sec = h->report_interval * 2},
		{.id = hbe_sensors[2].sensor_id, .status = THINGPLUS_STATUS_ON, .valid_duration_sec = h->report_interval * 2},
	};

	thingplus_status_publish(h->t, 4, thing_status);
	int v = hbe_sensors[0].ops.get(NULL);
	
	struct thingplus_value t;
	int temp = hbe_sensors[1].ops.get(NULL);
	t.value = calloc(1, 10);
	sprintf(t.value, "%d", temp);
	t.time_ms = _now_ms();
	thingplus_single_value_publish(h->t, hbe_sensors[1].sensor_id, &t);
	free(t.value);

	struct thingplus_value hu;
	int humidity = hbe_sensors[2].ops.get(NULL);
	hu.value = calloc(1, 10);
	sprintf(hu.value, "%d", humidity);
	hu.time_ms = _now_ms();
	thingplus_single_value_publish(h->t, hbe_sensors[2].sensor_id, &hu);
	free(hu.value);
}

static void hbe_event_trigger(void *arg, char *value)
{
	struct hbe *h = (struct hbe *)arg;
	printf("%s\n", __func__);
	int _value = (int)value;
	struct thingplus_value v;
	if (_value) 
		v.value = "1";
	else
		v.value = "0";
	v.time_ms = _now_ms();
	
	thingplus_single_value_publish(h->t, hbe_sensors[0].sensor_id, &v);
}

static void _connected(void *cb_arg, thingplus_error_e err)
{
	struct hbe *h = (struct hbe *)cb_arg;
	if (err) {
		printf("thingplus mqtt connection error:%d\n", err);
	}
	else {
		h->thingplus_connected = true;
	}
}

static void _disconnected(void *cb_arg, bool force)
{
	struct hbe *h = (struct hbe *)cb_arg;

	printf("thingplus mqtt disconnected. force:%d\n", force);

	h->thingplus_connected = false;
}

int main(int argc, char *argv[])
{
	struct hbe *h =  calloc(1, sizeof(struct hbe));
	if (h == NULL) {
		fprintf(stderr, "calloc failed\n");
		return 0;
	}
	h->report_interval = report_interval;

	struct thingplus_callback cb = {
		.connected = _connected,
	};


	int keep_alive = h->report_interval * 2;
	h->t = thingplus_init(gw_id, apikey, ca_file, keep_alive, &cb, h);
	if (h->t == NULL) {
		fprintf(stderr, "thingplus_init failed\n");
		return 0;
	}

	int ret = thingplus_connect_forever(h->t);
	if (ret < 0) {
		fprintf(stderr, "thingplus_connect_forever failed\n");
		return 0;
	}

	h->tdiscover = tdiscover_init(gw_id, apikey);
	if (h->tdiscover == NULL) {
		fprintf(stderr, "tdiscover_init failed\n");
		return 0;
	}

	int i, j;
	for (i=0; i<hbe.nr_device; i++) {
		tdiscover_device_register(h->tdiscover, hbe.devices[i].name, 
			hbe.devices[i].uid,
			hbe.devices[i].device_model_id,
			hbe.devices[i].device_id);
		for (j=0; j<hbe.devices[i].nr_sensor; j++) {
			tdiscover_sensor_register(h->tdiscover, 
				hbe.devices[i].sensors[j].name,
				hbe.devices[i].sensors[j].uid,
				hbe.devices[i].sensors[j].type,
				hbe.devices[i].device_id,
				hbe.devices[i].sensors[j].sensor_id);
		}
	}

	for (i=0; i<hbe.nr_device; i++) {
		for (j=0; j<hbe.devices[i].nr_sensor; j++) {
			if (hbe.devices[i].sensors[j].ops.init)
				hbe.devices[i].sensors[j].s = hbe.devices[i].sensors[j].ops.init((void*)h, hbe_event_trigger);
		}
	}

	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_signo = SIGUSR1;
	sev.sigev_value.sival_ptr = h;
	sev.sigev_notify_function = _value_status_publish;
	sev.sigev_notify_attributes = NULL;

	if (timer_create(CLOCK_REALTIME, &sev, &h->timer_id) < 0) {
		fprintf(stderr, "timer_create failed\n");
		return;
	}

	struct itimerspec its = {
		.it_interval.tv_sec = h->report_interval,
		.it_interval.tv_nsec = 0,
		.it_value.tv_sec = 1,
		.it_value.tv_nsec = 0,
	};

	timer_settime(h->timer_id, 0, &its, NULL);
	while(1);
		

	return 1;
}

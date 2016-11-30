#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thingplus.h>
#include <thingplus_types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "gateway.h"

#define REPORT_INTERVAL 60
#define KEEPALIVE	REPORT_INTERVAL

struct gw
{
	void *thingplus;
	char *id;
	bool connected;

	int nr_sensor;
	struct sensor *sensor;

	timer_t timer_id;

	pthread_mutex_t wait_thingplus_done_mutex;
	pthread_cond_t wait_thingplus_done;
};
static struct gw *gw = NULL;

static unsigned long long _now_ms(void)
{
        struct timeval tv;
        unsigned long long now;

        gettimeofday(&tv, NULL);
        now = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

        return now;
}

static void _thingplus_done(struct gw *g)
{
	pthread_mutex_lock(&g->wait_thingplus_done_mutex);
	pthread_cond_signal(&g->wait_thingplus_done);
	pthread_mutex_unlock(&g->wait_thingplus_done_mutex);
}

static void _thingplus_wait(struct gw *g)
{
	pthread_mutex_lock(&g->wait_thingplus_done_mutex);
	pthread_cond_wait(&g->wait_thingplus_done, &gw->wait_thingplus_done_mutex);
	pthread_mutex_unlock(&g->wait_thingplus_done_mutex);
}

static void _status_publish(union sigval sigval)
{
	struct gw *g = (struct gw*)sigval.sival_ptr;
	struct thingplus_status s[2];

	int i=0;

	for (i=0; i<g->nr_sensor; i++) {
		s[i].id = g->sensor[i].id;
		s[i].timeout_ms = (REPORT_INTERVAL * 2) * 1000;

		if (g->sensor[i].ops.on)
			s[i].status = g->sensor[i].ops.on() ? 0 : 1 ;
		else
			s[i].status = 1;
	}

	s[i].id = g->id;
	s[i].status = 0;
	s[i].timeout_ms = (REPORT_INTERVAL * 2) * 1000;

	thingplus_status_publish(g->thingplus, sizeof(s)/sizeof(s[0]), s);
}

static void _status_publish_timer_start(struct gw *g)
{
	struct itimerspec its;
	bzero(&its, sizeof(its));
	its.it_interval.tv_sec = REPORT_INTERVAL;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = 0;

	struct sigevent sev;
	bzero(&sev, sizeof(sev));
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_signo = SIGUSR1;
	sev.sigev_value.sival_ptr = g;
	sev.sigev_notify_function = _status_publish;
	sev.sigev_notify_attributes = NULL;

	if (timer_create(CLOCK_REALTIME, &sev, &g->timer_id) < 0) {
		fprintf(stderr, "timer_create failed\n");
		return;
	}

	timer_settime(g->timer_id, 0, &its, NULL);
}

static void _status_publish_timer_stop(struct gw *g)
{
	if (g->timer_id)
		timer_delete(g->timer_id);
}

static void _connected(void *arg, enum thingplus_error error)
{
	struct gw* g = (struct gw*)arg;
	g->connected = true;

	_status_publish_timer_start(g);
	_thingplus_done((struct gw*)arg);
}

int gw_event_value_publish(char *id, char *value)
{
	if (!gw)
		return -1;

	struct thingplus_value v;
	v.id = id;
	v.value = value;
	v.time_ms = _now_ms();

	return thingplus_value_publish(gw->thingplus, 1, &v);
}

int gw_connect(char *ca_cert)
{
	if (!gw)
		return -1;

	if (gw->connected)
		return 1;

	if (!ca_cert) {
		printf("[ERR] Invalid Argument\n");
		return -1;
	}

	thingplus_connect(gw->thingplus, ca_cert, KEEPALIVE);
	_thingplus_wait(gw);
	
	return 0;
}

void gw_disconnect(void)
{
	if (!gw)
		return;

	_status_publish_timer_stop(gw);
}

int gw_init(char *gw_id, char *apikey, int nr_sensor, struct sensor *s)
{
	if (gw) {
		printf("[WARN gw_connect is already called\n");
		return -1;
	}

	if (!gw_id || !apikey) {
		printf("[ERR] Invalid Argument\n");
		return -1;
	}

	gw = (struct gw*)calloc(1, sizeof(struct gw));
	if (gw == NULL) {
		return -1;
	}

	pthread_mutex_init(&gw->wait_thingplus_done_mutex, NULL);
	pthread_cond_init(&gw->wait_thingplus_done, NULL);

	gw->id = (char*)malloc(strlen(gw_id) + 1);
	if (!gw->id) 
		goto err_gw_id_dump;
	strcpy(gw->id, gw_id);

	gw->nr_sensor = nr_sensor;
	gw->sensor = calloc(nr_sensor, sizeof(struct sensor));
	if (!gw->sensor) {
		goto err_sensor_malloc;
	}
	memcpy(gw->sensor, s, nr_sensor * sizeof(struct sensor));

	gw->thingplus = thingplus_init(gw_id, apikey,
			"mqtt.thingplus.net", "https://api.thingplus.net");
	if (!gw->thingplus) {
		goto err_thingplus_init;
	}

	struct thingplus_callback cb;
	bzero(&cb, sizeof(struct thingplus_callback));
	cb.connected = _connected;
	thingplus_callback_set(gw->thingplus, &cb, gw);

	return 0;

err_thingplus_init:
	free(gw->sensor);

err_sensor_malloc:
	free(gw->id);

err_gw_id_dump:
	pthread_cond_destroy(&gw->wait_thingplus_done);
	pthread_mutex_destroy(&gw->wait_thingplus_done_mutex);

	free(gw);

	gw = NULL;
	return -1;

}

void gw_cleanup(void)
{
	if (!gw)
		return;

	gw_disconnect();

	if (gw->thingplus)
		thingplus_cleanup(gw->thingplus);

	if (gw->sensor)
		free(gw->sensor);

	if (gw->id)
		free(gw->id);

	pthread_cond_destroy(&gw->wait_thingplus_done);
	pthread_mutex_destroy(&gw->wait_thingplus_done_mutex);

	free(gw);
	gw = NULL;
}

#include <json-c/json.h>
#include <mosquitto.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "reassemble.h"
#include "service.h"
#include "thingplus.h"
#include "log/tube_log.h"

#define THINGPLUS_MAX_TOPIC 	256
#define THINGPLUS_MQTT_QOS	1
#define THINGPLUS_KEEPALIVE_MAX	(60 * 10)
#define RECONNECTION_DELAY	2
#define RECONNECTION_DELAY_MAX	(60 * 5)



#define min(x, y) (((x)  > (y)) ? (y) : (x))

struct mqtt_config {
	int port;
	int version;
	bool clean_session;
	struct {
		char topic[THINGPLUS_MAX_TOPIC];
		char* msg;
		int qos;
		bool retained;
	} will;
	int keepalive;
};

static struct mqtt_config THINGPLUS_MQTT_CONFIG = {
	.version = 3,
	.clean_session = true,
	.will.msg = "err",
	.will.qos = 1,
	.will.retained = true,
};

struct thingplus {
	struct mqtt_config config;

	enum {
		MQTT_STATE_DISCONNECTED,
		MQTT_STATE_CONNECTING,
		MQTT_STATE_CONNECTED,
	} state;

	char *gw_id;
	char *apikey;
	char topic_subscribe[THINGPLUS_MAX_TOPIC];
	char topic_response[THINGPLUS_MAX_TOPIC];

	struct thingplus_callback cb;
	void *cb_arg;

	pthread_t connection_tid;
	void *service;
	void *mosq;
};

static unsigned long long _now_ms(void)
{
	struct timeval tv;
	unsigned long long now;

	gettimeofday(&tv, NULL);
	now = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

	return now;
}

static void* _connect_forever(void* arg)
{
	struct thingplus *t = (struct thingplus *)arg;
	int ret;
	int retry = 0;

	while (1) {
		if (thingplus_connect((void*)t) == 0) {
			break;
		}
		sleep(1);
		//sleep(min(RECONNECTION_DELAY * retry++, RECONNECTION_DELAY_MAX));
	}

	pthread_exit(0);
}

static void _subscribing(struct thingplus *t)
{
	int ret = mosquitto_subscribe(t->mosq, NULL, t->topic_subscribe, THINGPLUS_MQTT_QOS);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("mosquitto_subscribe failed. %s\n", mosquitto_strerror(ret));
	}
}

static void _mqtt_status_publish(struct thingplus *t, bool connected)
{
	char topic[THINGPLUS_MAX_TOPIC] = {0,};
	sprintf(topic, "v/a/g/%s/mqtt/status", t->gw_id);

	char* payload = NULL;
	int payload_size = 0;
	char *value = connected ? "on" : "off";
	payload_size = reassemble_payload(&payload, payload_size, "%s", value);

	int ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, THINGPLUS_MQTT_QOS, false);
	if (ret != MOSQ_ERR_SUCCESS) 
		tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	reassemble_free(payload);
}

static void _on_publish(struct mosquitto *mosq,  void *obj, int mid)
{
	tube_log_info("published\tmid:%d\n", mid);
}

static void _on_connect(struct mosquitto *mosq, void *obj, int err)
{
	struct thingplus* t = (struct thingplus*)obj;

	if (err != 0) {
		tube_log_error("[MQTT] connection failed. err:%d\n", err);

		switch(err) {
		case 1:
			tube_log_error("[MQTT] unacceptable protocol version\n");
			t->cb.connected(t->cb_arg, 
				THINGPLUS_ERR_MQTT_PROTOCOL);
			break;
		case 2:
			tube_log_error("[MQTT] identifier rejected\n");
			t->cb.connected(t->cb_arg, 
				THINGPLUS_ERR_IDENTIFICATION);
			break;
		case 3:
			tube_log_error("[MQTT] broker unavailable\n");
			t->cb.connected(t->cb_arg, THINGPLUS_ERR_SERVER);
			break;
		case 4:
			tube_log_error("[MQTT] bad user name or password");
			t->cb.connected(t->cb_arg, THINGPLUS_ERR_IDENTIFICATION);
			break;
		case 5:
			tube_log_error("[MQTT] not authorized");
			t->cb.connected(t->cb_arg, THINGPLUS_ERR_AUTHORIZATION);
			break;
		default:
			tube_log_error("[MQTT] unknown error. %d\n", err);
			t->cb.connected(t->cb_arg, THINGPLUS_ERR_UNKNOWN);
			break;
		}
		return;
	}
	else {
		t->state = MQTT_STATE_CONNECTED;
		_mqtt_status_publish(t, true);
		_subscribing(t);

		if (t->cb.connected) 
			t->cb.connected(t->cb_arg, THINGPLUS_ERR_SUCCESS);
	}
}

static void _on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	tube_log_info("%s\n", (char*)msg->payload);

	struct thingplus* t = (struct thingplus*)obj;
	char *result = service_do(t->service, msg->payload);
	if (!result) {
		tube_log_error("service_do failed\n");
		return;
	}


	int ret = mosquitto_publish(t->mosq, NULL, t->topic_response, strlen(result), result, THINGPLUS_MQTT_QOS, false);
	if (ret != MOSQ_ERR_SUCCESS) 
		tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	service_result_free(result);
}

static void _on_disconnect(struct mosquitto *mosq, void *obj, int force)
{
	tube_log_info("disconnected. force:%d\n", force);

	struct thingplus* t = (struct thingplus*)obj;

	t->state = MQTT_STATE_DISCONNECTED;

	if (t->cb.disconnected) 
		t->cb.disconnected(t->cb_arg, force);
}

static void _on_log(struct mosquitto *mosq, void* obj, int level, const char* str)
{
	tube_log_info("%s\n", str);
}

enum thingplus_error thingplus_values_publish(void* instance, int nr_thing, struct thingplus_values *values)
{
	struct thingplus *t = (struct thingplus*)instance;

	if (t == NULL) {
		tube_log_error("[MQTT] instance is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (nr_thing == 0) {
		tube_log_error("[MQTT] nr_thing is 0\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (values == NULL) {
		tube_log_error("[MQTT] value is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (t->state != MQTT_STATE_CONNECTED) {
		tube_log_error("[MQTT] not connected\n");
		return THINGPLUS_ERR_NOT_CONNECTED;
	}

	char topic[THINGPLUS_MAX_TOPIC] = {0,};
	char* payload = NULL;
	int payload_size = 0;
	char* format;
	int i, j;

	if (nr_thing == 1) {
		sprintf(topic, "v/a/g/%s/s/%s", t->gw_id, values[0].id);

		format = "%lld,%s";
		for (i=0; i<values->nr_value; i++) {
			payload_size = reassemble_payload(&payload, payload_size, format, values->value[i].time_ms, values->value[i].value);
			format = ",%lld,%s";
		}
	}
	else {
		sprintf(topic, "v/a/g/%s", t->gw_id);

		format = "{\"%s\": [";
		for (i=0; i<nr_thing; i++) {
			if (!values[i].nr_value)
				continue;

			payload_size = reassemble_payload(&payload, payload_size, format, values[i].id);

			payload_size = reassemble_payload(&payload, payload_size, "%lld,%s", values[i].value[0].time_ms, values[i].value[0].value);
			for (j=1; j < values[i].nr_value; j++)
				payload_size = reassemble_payload(&payload, payload_size, ",%lld,%s", values[i].value[j].time_ms, values[i].value[j].value);

			payload_size = reassemble_payload(&payload, payload_size, "]");
			format = ", \"%s\": [";
		}
		payload_size = reassemble_payload(&payload, payload_size, "}");
	}

	tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);
	int ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, THINGPLUS_MQTT_QOS, false);
	if (ret != MOSQ_ERR_SUCCESS) 
		tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	reassemble_free(payload);

	return THINGPLUS_ERR_SUCCESS;
}

enum thingplus_error thingplus_single_value_publish(void* instance, char* id, struct thingplus_value *value)
{
	struct thingplus *t = (struct thingplus*)instance;

	if (t == NULL) {
		tube_log_error("[MQTT] instance is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (value == NULL) {
		tube_log_error("[MQTT] value is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (id == NULL) {
		tube_log_error("[MQTT] id is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (t->state != MQTT_STATE_CONNECTED) {
		tube_log_error("[MQTT] not connected\n");
		return THINGPLUS_ERR_NOT_CONNECTED;
	}

	char topic[THINGPLUS_MAX_TOPIC] = {0,};
	char* payload = NULL;
	int payload_size = 0;

	sprintf(topic, "v/a/g/%s/s/%s", t->gw_id, id);
	payload_size = reassemble_payload(&payload, payload_size, "%lld,%s", value->time_ms, value->value);

	tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);
	int ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, THINGPLUS_MQTT_QOS, false);
	if (ret != MOSQ_ERR_SUCCESS) 
		tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	reassemble_free(payload);

	return THINGPLUS_ERR_SUCCESS;
}

enum thingplus_error thingplus_status_publish(void *instance, int nr_thing, struct thingplus_status *things)
{
	struct thingplus *t = (struct thingplus*)instance;
	unsigned long long now = _now_ms();

	if (t == NULL) {
		tube_log_error("[MQTT] instance is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (things == NULL) {
		tube_log_error("[MQTT] things is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (t->state != MQTT_STATE_CONNECTED) {
		tube_log_error("[MQTT] not connected. state:%d\n", t->state);
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	int i;
	int gateway_status_index = -1;

	for (i=0; i<nr_thing; i++) {
		if (strcmp(things[i].id, t->gw_id) == 0) {
			gateway_status_index = i;
			break;
		}
	}

	char topic[THINGPLUS_MAX_TOPIC] = {0,};
	char* payload = NULL;
	int ret;
	int payload_size = 0;

	if (gateway_status_index != -1) {
		sprintf(topic, "v/a/g/%s/status", t->gw_id);

		payload_size = reassemble_payload(&payload, payload_size, "%s,%ld", 
				things[gateway_status_index].status == THINGPLUS_STATUS_ON ? "on" : "off",
				now + things[gateway_status_index].valid_duration_sec * 1000);
		for (i=0; i<nr_thing; i++) {
			if (i == gateway_status_index)
				continue;
			payload_size = reassemble_payload(&payload, payload_size, ",%s,%s,%ld", 
				things[i].id, 
				things[i].status == THINGPLUS_STATUS_ON ? "on" : "off",
				now + things[i].valid_duration_sec * 1000);
		}

		tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);

		ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, THINGPLUS_MQTT_QOS, false);
		if (ret != MOSQ_ERR_SUCCESS) 
			tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

		reassemble_free(payload);
	}
	else {
		for (i=0; i<nr_thing; i++) {
			memset(topic, 0, sizeof(topic));
			sprintf(topic, "v/a/g/%s/s/%s/status", t->gw_id, things[i].id);
			payload_size = reassemble_payload(&payload, payload_size, "%s,%ld", 
				things[i].status == THINGPLUS_STATUS_ON ? "on" : "off",
				now + things[i].valid_duration_sec * 1000);

			tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);

			ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, THINGPLUS_MQTT_QOS, false);
			if (ret != MOSQ_ERR_SUCCESS) {
				tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));
			}

			reassemble_free(payload);
			payload = NULL;
		}
	}

	return THINGPLUS_ERR_SUCCESS;
}

enum thingplus_error thingplus_mqtt_loop(void *instance)
{
	if (instance == NULL) {
		tube_log_error("instance is NULL");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct thingplus *t = (struct thingplus*)instance;

	int ret = mosquitto_loop(t->mosq, -1, 1);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("[MQTT] mosquitto_loop failed. %s\n",
				mosquitto_strerror(ret));

		switch (ret) {
		case MOSQ_ERR_NO_CONN:
		case MOSQ_ERR_CONN_LOST:
			return THINGPLUS_ERR_NOT_CONNECTED;
		case MOSQ_ERR_NOMEM:
			return THINGPLUS_ERR_NOMEM;
		default:
			return THINGPLUS_ERR_UNKNOWN;
		}
	}

	return THINGPLUS_ERR_SUCCESS;
}

enum thingplus_error thingplus_connect(void *instance)
{
	if (instance == NULL) {
		tube_log_error("instance is NULL");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct thingplus *t = (struct thingplus*)instance;

	if (t->state == MQTT_STATE_CONNECTING || t->state == MQTT_STATE_CONNECTED) {
		tube_log_warn("[MQTT] already connected");
		return THINGPLUS_ERR_SUCCESS;
	}

	int ret = mosquitto_connect_async(t->mosq, "dmqtt.thingplus.net", t->config.port, t->config.keepalive);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("[MQTT] mosquitto_connect_async failed. %s\n", mosquitto_strerror(ret));
		return THINGPLUS_ERR_NETWORK;
	}

	mosquitto_loop_start(t->mosq);

	return THINGPLUS_ERR_SUCCESS;
}

enum thingplus_error thingplus_connect_forever(void *instance)
{
	if (instance == NULL) {
		tube_log_error("instance is NULL");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct thingplus *t = (struct thingplus*)instance;

	if (thingplus_connect(instance) != THINGPLUS_ERR_SUCCESS) {
		if (pthread_create(&t->connection_tid, NULL, _connect_forever, t) < 0) {
			return THINGPLUS_ERR_THREAD;
		}
	}

	return THINGPLUS_ERR_SUCCESS;
}

#if 0
enum thingplus_error thingplus_connect(void *instance)
{
	if (instance == NULL) {
		tube_log_error("instance is NULL");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct thingplus *t = (struct thingplus*)instance;

	if (t->state == MQTT_STATE_CONNECTING || t->state == MQTT_STATE_CONNECTED) {
		tube_log_warn("[MQTT] already connected");
		return THINGPLUS_ERR_SUCCESS;
	}

	int ret = mosquitto_connect(t->mosq, "dmqtt.thingplus.net", t->config.port, t->config.keepalive);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("[MQTT] mosquitto_connect failed. %s\n", mosquitto_strerror(ret));
		return THINGPLUS_ERR_NETWORK;
	}

	return THINGPLUS_ERR_SUCCESS;
}
#endif

enum thingplus_error thingplus_disconnect(void *instance)
{
	if (instance == NULL) {
		tube_log_error("instance is NULL");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct thingplus *t = (struct thingplus*)instance;

	if (t->connection_tid) {
		pthread_cancel(t->connection_tid);
	}

	_mqtt_status_publish(t, false);
	mosquitto_unsubscribe(t->mosq, NULL, t->topic_subscribe);
	mosquitto_disconnect(t->mosq);
	mosquitto_loop_stop(t->mosq, false);

	return THINGPLUS_ERR_SUCCESS;
}

void* thingplus_init(char *gw_id, char *apikey, char *ca_file, int keepalive, struct thingplus_callback *cb, void *cb_arg)
{
	if (gw_id == NULL) {
		tube_log_error("[MQTT] gw_id is NULL\n");
		return NULL;
	}

	if (apikey == NULL) {
		tube_log_error("[MQTT] apikey is NULL\n");
		return NULL;
	}

	mosquitto_lib_init();
	tube_log_init(NULL);

	struct thingplus *t = calloc(1, sizeof(struct thingplus));
	if (t == NULL) {
		tube_log_error("[MQTT] calloc failed. size:%ld\n", 
			sizeof(struct thingplus));
		return NULL;
	}

	t->gw_id = strdup(gw_id);
	t->apikey = strdup(apikey);
	t->state = MQTT_STATE_DISCONNECTED;
	if (cb) {
		t->cb = *cb;
		t->cb_arg = cb_arg;
	}
	t->service = service_init(cb, cb_arg);

	t->config = THINGPLUS_MQTT_CONFIG;
	t->config.keepalive = min(keepalive, THINGPLUS_KEEPALIVE_MAX);
	sprintf(t->config.will.topic, "v/a/g/%s/mqtt/status",gw_id);

	sprintf(t->topic_subscribe, "v/a/g/%s/req",gw_id);
	sprintf(t->topic_response, "v/a/g/%s/res",gw_id);

	t->mosq = mosquitto_new(t->gw_id, true, (void*)t);
	if (!t->mosq) {
		tube_log_error("[MQTT] mosquitto_new failed\n");
		goto err_mosquitto_new;
	}

	mosquitto_reconnect_delay_set(t->mosq, RECONNECTION_DELAY, 
		RECONNECTION_DELAY_MAX, true);

	mosquitto_publish_callback_set(t->mosq, _on_publish);
	mosquitto_connect_callback_set(t->mosq, _on_connect);
	mosquitto_message_callback_set(t->mosq, _on_message);
	mosquitto_disconnect_callback_set(t->mosq, _on_disconnect);
	mosquitto_log_callback_set(t->mosq, _on_log);

	int ret = mosquitto_will_set(t->mosq, t->config.will.topic,
		strlen(t->config.will.msg),
		t->config.will.msg,
		t->config.will.qos,
		t->config.will.retained);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("[MQTT] mosquitto_will_set failed. %s\n", mosquitto_strerror(ret));
		goto err_mosquitto_will_set;
	}

	ret = mosquitto_username_pw_set(t->mosq, t->gw_id, t->apikey);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("[MQTT] mosquitto_username_pw_set failed. %s\n", mosquitto_strerror(ret));
		goto err_mosquitto_username_pw_set;
	}

	if (ca_file) {
		ret = mosquitto_tls_set(t->mosq, ca_file, NULL, NULL, NULL, NULL);
		if (ret != MOSQ_ERR_SUCCESS) {
			tube_log_error("[MQTT] mosquitto_tls_set failed. %s\n", mosquitto_strerror(ret));
			goto err_mosquitto_tls_set;
		}

		ret = mosquitto_tls_opts_set(t->mosq, 1, "tlsv1", NULL);
		if (ret != MOSQ_ERR_SUCCESS) {
			tube_log_error("[MQTT] mosquitto_tls_opts_set failed. %s\n", mosquitto_strerror(ret));
			goto err_mosquitto_tls_set;
		}

		t->config.port = 8883;
	}
	else
		t->config.port = 1883;

	return (void*) t;

err_mosquitto_tls_set:
err_mosquitto_username_pw_set:
	mosquitto_will_clear(t->mosq);

err_mosquitto_will_set:
	mosquitto_destroy(t->mosq);

err_mosquitto_new:
	service_cleanup(t->service);
	free(t->apikey);
	free(t->gw_id);
	free(t);

	return NULL;
}

void thingplus_cleanup(void *instance)
{
	struct thingplus *t = (struct thingplus*)instance;
	
	if (!t)
		return;

	if (t->state == MQTT_STATE_CONNECTED || t->state == MQTT_STATE_CONNECTING)
		thingplus_disconnect((void*)t);

	mosquitto_will_clear(t->mosq);
	mosquitto_destroy(t->mosq);

	service_cleanup(t->service);
	if (t->gw_id)
		free(t->gw_id);
	if (t->apikey)
		free(t->apikey);
	free(t);

	mosquitto_lib_cleanup();;

	return;
}

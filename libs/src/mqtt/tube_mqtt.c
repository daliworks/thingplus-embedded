#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

#include "tube_mqtt.h"
#include "log/tube_log.h"

#define TUBE_MQTT_MAX_TOPIC 256


struct mqtt_config {
	int version;
	bool clean_session;
	struct {
		char topic[TUBE_MQTT_MAX_TOPIC];
		char* msg;
		int qos;
		bool retained;
	} will;
};

struct tube_mqtt{ 
	struct mqtt_config config;

	enum {
		MQTT_DISCONNECTED,
		MQTT_CONNECTED,
	} mqtt_status;

	char* gw_id;
	char* apikey;

	struct tube_dm_ops dm_ops;
	int report_interval;

	struct tube_mqtt_callback callback;
	void* callbackb_arg;

	void* mosq;
};

static struct mqtt_config tube_default_config = {
	.version = 3,
	.will.msg = "err",
	.will.qos = 1,
	.will.retained = true,
};

static unsigned long long _current_time_ms_get(void)
{
	struct timeval tv;
	unsigned long long now;

	gettimeofday(&tv, NULL);
	now = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

	return now;
}

static char* _payload_add(char* payload, char* format, ...)
{
#define DEFAULT_MESSAGE_LEN 512

	char message_temp[DEFAULT_MESSAGE_LEN] = {0, };
	va_list args;

	va_start(args, format);
	vsprintf(message_temp, format, args);
	va_end(args);

	//tube_log_debug("message_temp:%s\n", message_temp);
	int message_size = strlen(message_temp);
	int payload_size = 0;
	if (payload)
		payload_size = strlen(payload);

	payload = realloc(payload, payload_size + message_size + 1);
	if (payload == NULL) {
		tube_log_error("[MQTT] realloc failed. size:%d\n", 
			payload_size + message_size + 1);
		return NULL;
	}

	strcpy(&payload[payload_size], message_temp);

	return payload;
}

static void _payload_del(char* payload)
{
	if (!payload)
		return;

	free(payload);
}

static char* _to_string_tube_mqtt_thing_status(enum tube_mqtt_thing_status s)
{
	if (s == TUBE_STATUS_ON)
		return "on";
	else
		return "off";
}


//TODO FIXME DELETE ME
static void on_log(struct mosquitto *mosq, void* obj, int level, const char* str)
{
	tube_log_info("%s\n", str);
}

static void _on_connect(struct mosquitto *mosq, void *obj, int err)
{
	struct tube_mqtt* t = (struct tube_mqtt*)obj;

	if (err != 0) {
		tube_log_error("[MQTT] connection failed\n");
		switch(err) {
		case 1:
			tube_log_error("[MQTT] unacceptable protocol version\n");
			t->callback.connect(t->callbackb_arg, 
				TUBE_MQTT_ERROR_UNKNOWN);
			return;
		case 2:
			tube_log_error("[MQTT] identifier rejected\n");
			t->callback.connect(t->callbackb_arg, 
				TUBE_MQTT_ERROR_IDENTIFICATION);
			return;
		case 3:
			tube_log_error("[MQTT] broker unavailable\n");
			t->callback.connect(t->callbackb_arg, 
				TUBE_MQTT_ERROR_BROKER);
			return;
		default:
			tube_log_error("[MQTT] unknown error\n");
			t->callback.connect(t->callbackb_arg, 
				TUBE_MQTT_ERROR_UNKNOWN);
			return;
		}
	}

	tube_log_info("[MQTT] connected\n");

	t->mqtt_status = MQTT_CONNECTED;
	if (t->callback.connect) 
		t->callback.connect(t->callbackb_arg, 
			TUBE_MQTT_ERROR_SUCCESS);
}

enum tube_mqtt_error tube_mqtt_status_send(void *instance, int nr_thing, struct tube_thing_status *things)
{
#define GATEWAY_STATUS_FOUND()		(gateway_status_index != -1)

	struct tube_mqtt *t = (struct tube_mqtt*)instance;
	unsigned long long now = _current_time_ms_get();

	if (t == NULL) {
		tube_log_error("[MQTT] instance is NULL\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (things == NULL) {
		tube_log_error("[MQTT] things is NULL\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (t->mqtt_status != MQTT_CONNECTED) {
		tube_log_error("[MQTT] not connected\n");
		return TUBE_MQTT_ERROR_NOT_CONNECTED;
	}

	int i;
	int gateway_status_index = -1;

	for (i=0; i<nr_thing; i++) {
		if (strcmp(things[i].id, t->gw_id) == 0) {
			gateway_status_index = i;
			break;
		}
	}

	char topic[TUBE_MQTT_MAX_TOPIC] = {0,};
	char* payload;
	int ret;

	if (GATEWAY_STATUS_FOUND()) {
		sprintf(topic, "v/a/g/%s/status", t->gw_id);

		payload = _payload_add(NULL, "%s,%ld", 
				_to_string_tube_mqtt_thing_status(things[gateway_status_index].status), 
				now + things[gateway_status_index].valid_duration_sec* 1000);

		for (i=0; i<nr_thing; i++) {
			if (i == gateway_status_index)
				continue;
			payload = _payload_add(payload, ",%s,%s,%ld", 
				things[i].id, 
				_to_string_tube_mqtt_thing_status(things[i].status),
				now + things[i].valid_duration_sec * 1000);
		}

		tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);

		ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, 1, false);
		if (ret != MOSQ_ERR_SUCCESS) 
			tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

		_payload_del(payload);
	}
	else {
		for (i=0; i<nr_thing; i++) {
			memset(topic, 0, sizeof(topic));
			sprintf(topic, "v/a/g/%s/s/%s/status", t->gw_id, things[i].id);
			payload = _payload_add(NULL, "%s,%ld", 
				_to_string_tube_mqtt_thing_status(things[i].status),
				now + things[i].valid_duration_sec * 1000);

			tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);

			ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, 1, false);
			if (ret != MOSQ_ERR_SUCCESS) {
				tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));
			}
			_payload_del(payload);
			payload = NULL;
		}
	}

	return TUBE_MQTT_ERROR_SUCCESS;
}

enum tube_mqtt_error tube_mqtt_single_value_send(void* instance, char* id, struct tube_thing_value *value)
{
	struct tube_mqtt *t = (struct tube_mqtt*)instance;

	if (t == NULL) {
		tube_log_error("[MQTT] instance is NULL\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (value == NULL) {
		tube_log_error("[MQTT] value is NULL\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (t->mqtt_status != MQTT_CONNECTED) {
		tube_log_error("[MQTT] not connected\n");
		return TUBE_MQTT_ERROR_NOT_CONNECTED;
	}

	char topic[TUBE_MQTT_MAX_TOPIC] = {0,};
	char* payload;

	sprintf(topic, "v/a/g/%s/s/%s", t->gw_id, id);
	payload = _payload_add(NULL, "%lld,%s", value->time_ms, value->value);

	tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);
	int ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, 1, false);
	if (ret != MOSQ_ERR_SUCCESS) 
		tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	_payload_del(payload);

	return TUBE_MQTT_ERROR_SUCCESS;
}

enum tube_mqtt_error tube_mqtt_values_send(void* instance, int nr_thing, struct tube_thing_values *values)
{
	struct tube_mqtt *t = (struct tube_mqtt*)instance;

	if (t == NULL) {
		tube_log_error("[MQTT] instance is NULL\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (nr_thing == 0) {
		tube_log_error("[MQTT] nr_thing is 0\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (values == NULL) {
		tube_log_error("[MQTT] value is NULL\n");
		return TUBE_MQTT_ERROR_INVALID_ARGUMENT;
	}

	if (t->mqtt_status != MQTT_CONNECTED) {
		tube_log_error("[MQTT] not connected\n");
		return TUBE_MQTT_ERROR_NOT_CONNECTED;
	}

	char topic[TUBE_MQTT_MAX_TOPIC] = {0,};
	char* payload = NULL;
	char* format;
	int i, j;

	if (nr_thing == 1) {
		sprintf(topic, "v/a/g/%s/s/%s", t->gw_id, values[0].id);

		format = "%lld,%s";
		for (i=0; i<values->nr_value; i++) {
			payload = _payload_add(payload, format, values->value[i].time_ms, values->value[i].value);
			format = ",%lld,%s";
		}
	}
	else {
		sprintf(topic, "v/a/g/%s", t->gw_id);

		format = "{\"%s\": [";
		for (i=0; i<nr_thing; i++) {
			if (!values[i].nr_value)
				continue;

			payload = _payload_add(payload, format, values[i].id);

			payload = _payload_add(payload, "%lld,%s", values[i].value[0].time_ms, values[i].value[0].value);
			for (j=1; j < values[i].nr_value; j++)
				payload = _payload_add(payload, ",%lld,%s", values[i].value[j].time_ms, values[i].value[j].value);

			payload = _payload_add(payload, "]");
			format = ", \"%s\": [";
		}
		payload = _payload_add(payload, "}");
	}

	tube_log_debug("[MQTT] publish. topic:%s payload:%s\n", topic, payload);
	int ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, 1, false);
	if (ret != MOSQ_ERR_SUCCESS) 
		tube_log_error("mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	_payload_del(payload);

	return 0;
}

void* tube_mqtt_connect(char* gw_id, char* apikey, char* ca_file, int report_interval, struct tube_mqtt_callback *mqtt_cb, void* cb_arg, char* logfile)
{
	int port = 1883;

	mosquitto_lib_init();

	tube_log_init(logfile);

	if (gw_id == NULL) {
		tube_log_error("[MQTT] gw_id is NULL\n");
		return NULL;
	}

	if (apikey == NULL) {
		tube_log_error("[MQTT] apikey is NULL\n");
		return NULL;
	}

	struct tube_mqtt *t = calloc(1, sizeof(struct tube_mqtt));
	if (t == NULL) {
		tube_log_error("[MQTT] calloc failed. size:%ld\n", 
			sizeof(struct tube_mqtt));
		return NULL;
	}

	t->callback = *mqtt_cb;
	t->callbackb_arg = cb_arg;

	t->gw_id = strdup(gw_id);
	t->apikey = strdup(apikey);
	t->report_interval = report_interval;

	t->config = tube_default_config;
	sprintf(t->config.will.topic, "v/a/g/%s/mqtt/status",gw_id);

	t->mosq = mosquitto_new(t->gw_id, true, (void*)t);
	if (!t->mosq) {
		tube_log_error("[MQTT] mosquitto_new failed\n");
		goto err_mosquitto_new;
	}

	mosquitto_log_callback_set(t->mosq, on_log);

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

#warning "fix mqtt port 8889 to 8883"
		port = 8889;
	}

	mosquitto_connect_callback_set(t->mosq, _on_connect);
#warning "fix mqtt host"
	ret = mosquitto_connect(t->mosq, "mqtt.testyh.thingbine.com", port, 60 * 10);
	if (ret != MOSQ_ERR_SUCCESS) {
		tube_log_error("[MQTT] mosquitto_connect failed. %s\n", mosquitto_strerror(ret));
		goto err_mosquitto_connect;
	}

	mosquitto_loop_start(t->mosq);

	return (void*) t;

err_mosquitto_connect:
err_mosquitto_tls_set:
err_mosquitto_username_pw_set:
	mosquitto_will_clear(t->mosq);

err_mosquitto_will_set:
	mosquitto_destroy(t->mosq);

err_mosquitto_new:
	free(t->apikey);
	free(t->gw_id);
	free(t);

	return NULL;
}

void tube_mqtt_disconnect(void *instance)
{
	struct tube_mqtt *t = (struct tube_mqtt *)instance;
	
	if (!t) {
		return;
	}

	mosquitto_disconnect(t->mosq);
	mosquitto_loop_stop(t->mosq, false);
	mosquitto_will_clear(t->mosq);
	mosquitto_destroy(t->mosq);

	if (t->gw_id)
		free(t->gw_id);

	if (t->apikey)
		free(t->apikey);

	free(t);
	return;
}

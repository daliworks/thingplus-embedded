#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <thingplus_types.h>

#include "device_management.h"
#include "mqtt.h"
#include "packetize.h"
#include "values.h"

#define min(a, b) ((a) > (b) ? (b) : (a))

#define MQTT_QOS	1
#define TOPIC_LENGTH	256

struct mqtt {
	char *gw_id;
	char *apikey;
	char *mqtt_url;
	int keepalive;

	char topic_subscribe[TOPIC_LENGTH];
	char topic_response[TOPIC_LENGTH];

	struct thingplus_callback callback;
	void *callback_arg;

	void *device_management;

	enum {
		STATE_INITIALIZED = 0,
		STATE_CONNECTED = 1,
		STATE_DISCONNECTED = 2,
	} state;

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

static int _keepalive_set(int user_keepalive)
{
#define KEEPALIVE_MAXIMUM_SEC	(60 * 10)
#define KEEPALIVE_MINIMUM_SEC	(60)

	if (user_keepalive <= 0)
		return KEEPALIVE_MINIMUM_SEC;
	else
		return min(user_keepalive, KEEPALIVE_MAXIMUM_SEC);
}

static bool _file_exist(char *file)
{
	struct stat st;
	return !stat(file, &st);
}

static bool _will_set(void *mosq, char *gw_id)
{
	char* message = "err";
	char topic[256] = {0,};
	sprintf(topic, "v/a/g/%s/mqtt/status", gw_id);

	int ret = mosquitto_will_set(mosq, topic, strlen(message), message, 1, true);
	if (ret != MOSQ_ERR_SUCCESS) {
		fprintf(stdout, "[ERR] mosquitto_will_set failed. topic:%s %s\n", 
				topic, mosquitto_strerror(ret));
		return false;
	}

	return true;
}

static void _mqtt_status_publish(struct mqtt *t, bool connected)
{
	char topic[TOPIC_LENGTH] = {0,};
	sprintf(topic, "v/a/g/%s/mqtt/status", t->gw_id);

	char* payload = NULL;
	int payload_size = 0;
	char *value = connected ? "on" : "off";
	payload_size = packetize_payload(&payload, payload_size, "%s", value);

	int ret = mosquitto_publish(t->mosq, NULL, topic, strlen(payload), payload, MQTT_QOS, false);
	if (ret != MOSQ_ERR_SUCCESS)
		fprintf(stdout, "[ERR] mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	packetize_free(payload);
}

static bool _values_contain_single_sensor(int nr_value, struct thingplus_value *values)
{
	if (nr_value == 1)
		return true;

	void *id = values[0].id;

	int i;
	for (i=1; i<nr_value; i++) {
		if (!values[i].id)
			continue;
		if (values[i].id != id)
			return false;
	}

	return true;
}

static int _sensor_value_packetize(char **payload, int payload_size, int nr_value, struct thingplus_value *v)
{
	uint64_t time_ms;
	char *format = "%lld,%s";
	int i;
	for (i=0; i<nr_value; i++) {
		if (!v[i].id)
			continue;

		time_ms = v[i].time_ms ? v[i].time_ms : _now_ms();
		payload_size = packetize_payload(payload, payload_size, format, time_ms, v[i].value);
		format = ",%lld,%s";
	}

	return payload_size;
}

static int _sensors_value_packetize(char **payload, int payload_size, int nr_value, struct thingplus_value *values)
{
	void *s = values_init(nr_value, values);
	if (s == NULL) {
		fprintf(stdout, "[ERR] values_init falied\n");
		return 0;
	}

	char *format = "{\"%s\": [";
	struct values_sensor *values_sensor = NULL;
	values_for_each_sensor(values_sensor, s) {
		if (!values_sensor->id)
			continue;

		payload_size = packetize_payload(payload, payload_size, format, values_sensor->id);
		payload_size = _sensor_value_packetize(payload, payload_size, values_sensor->nr_value, values_sensor->value);
		payload_size = packetize_payload(payload, payload_size, "]");
		format = ", \"%s\": [";
	}
	payload_size = packetize_payload(payload, payload_size, "}");

	values_cleanup(s);

	return payload_size;
}

static void _cb_mosquitto_log(struct mosquitto *mosq, void *obj, int level, const char *log)
{
	fprintf(stdout, "-------------------------\n");
	fprintf(stdout, log);
	fprintf(stdout, "\n");
}

static void _cb_connected(struct mosquitto *mosq, void *obj, int err_mosquitto)
{
	fprintf(stdout, "[INFO] connect result:%d %s\n", err_mosquitto,
			mosquitto_connack_string(err_mosquitto));

	struct mqtt *t = obj;

	if (!err_mosquitto) {
		t->state = STATE_CONNECTED;

		int ret = mosquitto_subscribe(t->mosq, NULL, t->topic_subscribe, MQTT_QOS);
		if (ret != MOSQ_ERR_SUCCESS) {
			fprintf(stdout, "[ERR] mosquitto_subscribe failed. %s\n", mosquitto_strerror(ret));
		}

		_mqtt_status_publish(t, true);
	}

	if (t->callback.connected) {
		enum thingplus_error errno;

		switch (err_mosquitto) {
			case 0:
				errno = THINGPLUS_ERR_SUCCESS;
				break;
			case 1:
				errno = THINGPLUS_ERR_MQTT_PROTOCOL;
				break;
			case 2:
				errno = THINGPLUS_ERR_IDENTIFICATION;
				break;
			case 3:
				errno = THINGPLUS_ERR_SERVER;
				break;
			case 4:
				errno = THINGPLUS_ERR_IDENTIFICATION;
				break;
			case 5:
				errno = THINGPLUS_ERR_AUTHORIZATION;
				break;
			default:
				errno = THINGPLUS_ERR_UNKNOWN;
				break;
		}

		t->callback.connected(t->callback_arg, errno);
	}
}

static void _cb_disconnected(struct mosquitto *mosq, void *obj, int err_mosquitto)
{
	fprintf(stdout, "[INFO] mqtt disconnected:%d\n", err_mosquitto);

	struct mqtt *t = obj;
	t->state = STATE_DISCONNECTED;

	if (t->callback.disconnected) {
		enum thingplus_error error;
		error = err_mosquitto ? THINGPLUS_ERR_UNEXPECTED : THINGPLUS_ERR_SUCCESS;

		t->callback.disconnected(t->callback_arg, error);
	}
}

static void _cb_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	struct mqtt *t = obj;
	char *result = device_management_do(t->device_management, msg->payload);
	if (!result) {
		fprintf(stdout, "[ERR] device_management_do failed\n");
		return;
	}

	int ret = mosquitto_publish(t->mosq, NULL, t->topic_response, strlen(result), result, MQTT_QOS, false);
	if (ret != MOSQ_ERR_SUCCESS)
		fprintf(stdout, "[ERR] mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

	device_management_result_free(result);
}

static void _cb_publish(struct mosquitto *mosq, void *obj, int mid)
{
	fprintf(stdout, "[INFO] message published. %d\n", mid);
	struct mqtt *t = obj;

	if (t->callback.published) {
		t->callback.published(t->callback_arg, mid);
	}
}

int mqtt_value_publish(void *_t, int *mid, int nr_value, struct thingplus_value *values)
{
	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (nr_value <= 0) {
		fprintf(stdout, "invalid nr_value\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (values == NULL) {
		fprintf(stdout, "values is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct mqtt *t = _t;

	if (t->state != STATE_CONNECTED) {
		fprintf(stdout, "mqtt is not connected\n");
		return THINGPLUS_ERR_NOT_CONNECTED;
	}

	int ret = 0;
	char topic[TOPIC_LENGTH] = {0,};
	char *payload = NULL;
	int payload_size = 0;

	if (_values_contain_single_sensor(nr_value, values)) {
		sprintf(topic, "v/a/g/%s/s/%s", t->gw_id, values[0].id);
		payload_size = _sensor_value_packetize(&payload, payload_size, nr_value, values);
	}
	else {
		sprintf(topic, "v/a/g/%s", t->gw_id);
		_sensors_value_packetize(&payload, payload_size, nr_value, values);
	}

	fprintf(stdout, "[DEBUG] publish. topic:%s payload:%s\n", topic, payload);

	int mosq_err = mosquitto_publish(t->mosq, mid, topic, strlen(payload), payload, MQTT_QOS, false);
	if (mosq_err != MOSQ_ERR_SUCCESS) {
		fprintf(stdout, "[ERR] mosquitto_publish failed. %s\n", mosquitto_strerror(mosq_err));
		ret = -1;
	}

	packetize_free(payload);

	return ret;
}

int mqtt_status_publish(void* _t, int *mid, int nr_thing, struct thingplus_status *things)
{
	if (_t == NULL) {
		fprintf(stdout, "[ERR] invalid thingplus instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (nr_thing <= 0) {
		fprintf(stdout, "invalid nr_thing\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	if (things == NULL) {
		fprintf(stdout, "things is NULL\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct mqtt *t = _t;

	if (t->state != STATE_CONNECTED) {
		fprintf(stdout, "mqtt is not connected\n");
		return THINGPLUS_ERR_NOT_CONNECTED;
	}

	int i;
	int gateway_status_index = -1;

	for (i=0; i<nr_thing; i++) {
		if (things[i].id && strcmp(things[i].id, t->gw_id) == 0) {
			gateway_status_index = i;
			break;
		}
	}

	unsigned long long now = _now_ms();
	char topic[TOPIC_LENGTH] = {0,};
	char* payload = NULL;
	int ret;
	int payload_size = 0;

	if (gateway_status_index != -1) {
		sprintf(topic, "v/a/g/%s/status", t->gw_id);

		payload_size = packetize_payload(&payload, payload_size, "%s,%ld",
				things[gateway_status_index].status == THINGPLUS_STATUS_ON ? "on" : "off",
				now + things[gateway_status_index].timeout_ms);
		for (i=0; i<nr_thing; i++) {
			if (i == gateway_status_index || !things[i].id)
				continue;
			payload_size = packetize_payload(&payload, payload_size, ",%s,%s,%ld",
					things[i].id,
					things[i].status == THINGPLUS_STATUS_ON ? "on" : "off",
					now + things[i].timeout_ms);
		}

		fprintf(stdout, "[DEBUG] publish. topic:%s payload:%s\n", topic, payload);

		ret = mosquitto_publish(t->mosq, mid, topic, strlen(payload), payload, MQTT_QOS, false);
		if (ret != MOSQ_ERR_SUCCESS)
			fprintf(stdout, "[ERR] mosquitto_publish failed. %s\n", mosquitto_strerror(ret));

		packetize_free(payload);
	}
	else {
		for (i=0; i<nr_thing; i++) {
			if (!things[i].id)
				continue;

			memset(topic, 0, sizeof(topic));
			sprintf(topic, "v/a/g/%s/s/%s/status", t->gw_id, things[i].id);
			payload_size = packetize_payload(&payload, payload_size, "%s,%ld",
					things[i].status == THINGPLUS_STATUS_ON ? "on" : "off",
					now + things[i].timeout_ms);

			fprintf(stdout, "[DEBUG] publish. topic:%s payload:%s\n", topic, payload);

			ret = mosquitto_publish(t->mosq, mid, topic, strlen(payload), payload, MQTT_QOS, false);
			if (ret != MOSQ_ERR_SUCCESS) {
				fprintf(stdout, "[ERR] mosquitto_publish failed. %s\n", mosquitto_strerror(ret));
			}

			packetize_free(payload);
			payload_size = 0;
			payload = NULL;
		}
	}

	return THINGPLUS_ERR_SUCCESS;
}


int mqtt_disconnect(void *_t)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid mqtt instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct mqtt *t = (struct mqtt*)_t;
	if (t->state != STATE_INITIALIZED) {
		mosquitto_unsubscribe(t->mosq, NULL, t->topic_subscribe);
	}

	if (t->state == STATE_CONNECTED) {
		_mqtt_status_publish(t, false);
	}

	mosquitto_disconnect(t->mosq);
	mosquitto_loop_stop(t->mosq, false);

	return 0;
}

int mqtt_connect(void *_t, int port, char *ca_file, int keepalive)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid mqtt instance\n");
		return THINGPLUS_ERR_INVALID_ARGUMENT;
	}

	struct mqtt *t = (struct mqtt*)_t;
	int ret = -1;

	if (ca_file) {
		if (!_file_exist(ca_file)) {
			fprintf(stdout, "[ERR] ca_file isn`t exist. file:%s\n", ca_file);
			goto err_ca_file_exist;
		}

		ret = mosquitto_tls_set(t->mosq, ca_file, NULL, NULL, NULL, NULL);
		if (ret != MOSQ_ERR_SUCCESS) {
			fprintf(stdout, "[ERR] mosquitto_tls_set failed. %s\n", 
				mosquitto_strerror(ret));
			goto err_mosquitto_tls_set;
		}

		ret = mosquitto_tls_opts_set(t->mosq, 1, "tlsv1", NULL);
		if (ret != MOSQ_ERR_SUCCESS) {
			fprintf(stdout, "[ERR] mosquitto_tls_opts_set failed. %s\n", 
				mosquitto_strerror(ret));
			goto err_mosquitto_tls_opts_set;
		}
	}

	mosquitto_connect_callback_set(t->mosq, _cb_connected);
	mosquitto_disconnect_callback_set(t->mosq, _cb_disconnected);
	mosquitto_message_callback_set(t->mosq, _cb_message);
	mosquitto_publish_callback_set(t->mosq, _cb_publish);

	ret = mosquitto_connect_async(t->mosq, t->mqtt_url, port, _keepalive_set(keepalive));
	if (ret != MOSQ_ERR_SUCCESS) {
		fprintf(stdout, "[ERR] mosquitto_connect_async failed. %s\n", 
				mosquitto_strerror(ret));
		goto err_mosquitto_connect_async;
	}
	mosquitto_loop_start(t->mosq);

	fprintf(stdout, "[INFO] mqtt server:%s port:%d\n", t->mqtt_url, port);

	return 0;

err_mosquitto_connect_async:
err_mosquitto_tls_opts_set:
err_mosquitto_tls_set:
err_ca_file_exist:
	return -1;
}

void mqtt_callback_set(void *_t, struct thingplus_callback *callback, void *callback_arg)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid mqtt instance\n");
		return;
	}

	struct mqtt *t = (struct mqtt*)_t;
	t->callback = *callback;
	t->callback_arg = callback_arg;
	if (t->device_management)
		device_management_cleanup(t->device_management);

	struct device_management_ops ops;
	ops.actuating = callback->actuating;
	ops.timesync = callback->timesync;
	ops.property_set = callback->property_set;
	ops.poweroff = callback->poweroff;
	ops.reboot = callback->reboot;
	ops.restart = callback->restart;
	ops.upgrade = callback->upgrade;
	ops.version = callback->version;

	t->device_management = device_management_init(&ops, callback_arg);
}

void *mqtt_init(char *gw_id, char *apikey, char *mqtt_url)
{
	int ret = 0;
	if (!gw_id) {
		fprintf(stdout, "[ERR] invalid gw_id\n");
		return NULL;
	}

	if (!apikey) {
		fprintf(stdout, "[ERR] invalid apikey\n");
		return NULL;
	}

	if (!mqtt_url) {
		fprintf(stdout, "[ERR] invalid mqtt_url\n");
		return NULL;
	}

	struct mqtt *t = calloc(1, sizeof(struct mqtt));
	if (t == NULL) {
		fprintf(stdout, "[ERR] calloc failed. size:%ld\n", sizeof(struct mqtt));
		return NULL;
	}

	sprintf(t->topic_subscribe, "v/a/g/%s/req", gw_id);
	sprintf(t->topic_response, "v/a/g/%s/res", gw_id);

	mosquitto_lib_init();

	t->mosq = mosquitto_new(gw_id, true, (void*)t);
	if (!t->mosq) {
		fprintf(stdout, "[ERR] mosquitto_new failed\n");
		goto err_mosquitto_new;
	}
	
	mosquitto_log_callback_set(t->mosq, _cb_mosquitto_log);

#define RECONNECT_DELAY (2)
#define RECONNECT_DELAY_MAX (60 * 5)
	ret = mosquitto_reconnect_delay_set(t->mosq, RECONNECT_DELAY, RECONNECT_DELAY_MAX, true);
	if (ret != MOSQ_ERR_SUCCESS) {
		fprintf(stdout, "[ERR] mosquitto_reconnect_delay_set failed. %s\n",
			mosquitto_strerror(ret));
		goto err_mosquitto_reconnect_delay_set;
	}

	ret = mosquitto_username_pw_set(t->mosq, gw_id, apikey);
	if (ret != MOSQ_ERR_SUCCESS) {
		fprintf(stdout, "[ERR] mosquitto_username_pw_set failed. %s\n",
			mosquitto_strerror(ret));
		goto err_mosquitto_username_pw_set;
	}

	if (!_will_set(t->mosq, gw_id)) {
		fprintf(stdout, "[ERR] _will_set failed\n");
		goto err_will_set;
	}

	t->state = STATE_INITIALIZED;
	t->gw_id = strdup(gw_id);
	t->apikey = strdup(apikey);
	t->mqtt_url = strdup(mqtt_url);

	fprintf(stdout, "[INFO] mqtt_init gw_id:%s apikey:%s\n", gw_id, apikey);

	return  (void*)t;

err_will_set:
err_mosquitto_username_pw_set:
err_mosquitto_reconnect_delay_set:
	mosquitto_destroy(t->mosq);
	t->mosq = NULL;

err_mosquitto_new:
	mosquitto_lib_cleanup();
	free(t);
	return NULL;
}

void mqtt_cleanup(void *_t)
{
	if (!_t) {
		fprintf(stdout, "[ERR] invalid mqtt instance\n");
		return;
	}

	struct mqtt *t = (struct mqtt*)_t;

	if (t->gw_id)
		free(t->gw_id);

	if (t->apikey)
		free(t->apikey);

	if (t->mqtt_url)
		free(t->mqtt_url);

	if (t->device_management)
		device_management_cleanup(t->device_management);

	mosquitto_destroy(t->mosq);
	mosquitto_lib_cleanup();

	free(t);
}

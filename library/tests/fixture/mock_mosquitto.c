#include <stdio.h>
#include <stdbool.h>
#include <CppUTestExt/MockSupport_c.h>

#include "mock_mosquitto.h"


struct mosquitto_message{
	int mid;
	char *topic;
	void *payload;
	int payloadlen;
	int qos;
	bool retain;
};

struct mosquitto;

static const char *gw_id = NULL;

static void *_cb_arg = NULL;
static void (*_cb_connect)(struct mosquitto *, void *, int) = NULL;
static void (*_cb_disconnect)(struct mosquitto *, void *, int) = NULL;
static void (*_cb_message)(struct mosquitto *, void *, const struct mosquitto_message *);

void mock_mosquitto_connected(int result)
{
	if (_cb_connect)
		return _cb_connect(NULL, _cb_arg, result);
}

void mock_mosquitto_disconnected(int result)
{
	if (_cb_disconnect)
		return _cb_disconnect(NULL, _cb_arg, result);
}

void mock_mosquitto_subscribed(char *topic, char *payload, int payload_len)
{
	if (_cb_message) {
		struct mosquitto_message message;
		message.topic = topic;
		message.payload = payload;
		message.payloadlen = payload_len;
		_cb_message(NULL, _cb_arg, &message);
	}
}

int mosquitto_lib_init(void)
{
	mock_c()->actualCall("mosquitto_lib_init");
	return 0;
}

int mosquitto_lib_cleanup(void)
{
	mock_c()->actualCall("mosquitto_lib_cleanup");
	return 0;
}

struct mosquitto *mosquitto_new(const char * id, bool clean_session, void *obj)
{
	_cb_arg = obj;
	mock_c()->actualCall("mosquitto_new")
		->withStringParameters("id", id);

	return mock_c()->returnValue().value.pointerValue;
}

void mosquitto_destroy(struct mosquitto * mosq)
{
	mock_c()->actualCall("mosquitto_destroy");
	return;
}

int mosquitto_username_pw_set(struct mosquitto * mosq, const char * username, const char * password)
{
	mock_c()->actualCall("mosquitto_username_pw_set")
		->withStringParameters("username", username)
		->withStringParameters("password", password);

	gw_id = username;

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_reconnect_delay_set(struct mosquitto * mosq, unsigned int reconnect_delay,
		unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
{
	mock_c()->actualCall("mosquitto_reconnect_delay_set")
		->withUnsignedIntParameters("reconnect_delay", reconnect_delay)
		->withUnsignedIntParameters("reconnect_delay_max", reconnect_delay_max);

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_connect_async(struct mosquitto* mosq, const char *host, int port, int keepalive) 
{
	mock_c()->actualCall("mosquitto_connect_async")
		->withStringParameters("host", host)
		->withIntParameters("port", port)
		->withIntParameters("keepalive", keepalive);

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_disconnect(struct mosquitto * mosq)
{
	mock_c()->actualCall("mosquitto_disconnect");

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_loop_start(struct mosquitto * mosq)
{
	mock_c()->actualCall("mosquitto_loop_start");

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_loop_stop(struct mosquitto * mosq, bool force)
{
	mock_c()->actualCall("mosquitto_loop_stop");

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic,
		int payloadlen, const char *payload, int qos, bool retain)
{
	mock_c()->actualCall("mosquitto_publish")
		->withStringParameters("topic", topic)
		->withStringParameters("payload", payload)
		->withIntParameters("payloadlen", payloadlen);

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
	mock_c()->actualCall("mosquitto_subscribe")
		->withStringParameters("sub", sub)
		->withIntParameters("qos", qos);

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, char *sub)
{
	mock_c()->actualCall("mosquitto_unsubscribe")
		->withStringParameters("sub", sub);

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_tls_set(struct mosquitto * mosq, const char * cafile, const char * capath,
		const char * certfile, const char * keyfile,
		int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	mock_c()->actualCall("mosquitto_tls_set")
		->withStringParameters("cafile", cafile);

	return mock_c()->returnValue().value.intValue;
}

int mosquitto_tls_opts_set(struct mosquitto * mosq, int cert_reqs, const char * tls_version,
		const char * ciphers)
{
	return 0;
}


const char *mosquitto_strerror(int mosq_errno)
{
	return "dummy";
}

const char *mosquitto_connack_string(int connack_code)
{
	return "";
}


void mosquitto_connect_callback_set(struct mosquitto * mosq,
		void (*on_connect)(struct mosquitto *, void *, int))
{
	_cb_connect = on_connect;
	mock_c()->actualCall("mosquitto_connect_callback_set");
}

void mosquitto_disconnect_callback_set(struct mosquitto * mosq,
		void (*on_disconnect)(struct mosquitto *, void *, int))
{
	_cb_disconnect = on_disconnect;
	mock_c()->actualCall("mosquitto_disconnect_callback_set");
}

void mosquitto_message_callback_set(struct mosquitto *mosq,
		void (*on_message)(struct mosquitto *, void*, const struct mosquitto_message*))
{
	_cb_message = on_message;
	mock_c()->actualCall("mosquitto_message_callback_set");
}


void mosquitto_log_callback_set(struct mosquitto * mosq,
		void (*on_log)(struct mosquitto *, void *, int, const char *))
{
}


int mosquitto_will_set(struct mosquitto * mosq, const char * topic, int payloadlen, const void * payload, int qos, bool retain)
{
	return 0;
}

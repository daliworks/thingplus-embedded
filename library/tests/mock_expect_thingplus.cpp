#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#include "mock_expect_tcurl.h"

extern "C" {
#include <stdio.h>
#include <string.h>
}


void mock_expect_thingplus_init(char *gw_id, char *apikey)
{
	mock().expectOneCall("mosquitto_lib_init");
	mock().expectOneCall("mosquitto_new")
		.withParameter("id", gw_id)
		.andReturnValue((void*)0xbeefffff);
	mock().expectOneCall("mosquitto_reconnect_delay_set")
		.withParameter("reconnect_delay", 2)
		.withParameter("reconnect_delay_max", (60 * 5));
	mock().expectOneCall("mosquitto_username_pw_set")
		.withParameter("username", gw_id)
		.withParameter("password", apikey);
	mock_expect_tcurl_init();
}

void mock_expect_thingplus_cleanup(void)
{
	mock().expectOneCall("mosquitto_destroy");
	mock().expectOneCall("mosquitto_lib_cleanup");
	mock_expect_tcurl_cleanup();
}

void mock_expect_thingplus_connect_ssl(char *ca_file, char *mqtt_url, int keepalive)
{
	mock().expectOneCall("mosquitto_message_callback_set");
	mock().expectOneCall("mosquitto_connect_callback_set");
	mock().expectOneCall("mosquitto_disconnect_callback_set");
	mock().expectOneCall("mosquitto_tls_set")
		.withParameter("cafile", ca_file);
	mock().expectOneCall("mosquitto_connect_async")
		.withParameter("host", mqtt_url)
		.withParameter("keepalive", keepalive)
		.withParameter("port", 8883);

	mock().expectOneCall("mosquitto_loop_start");
}

void mock_expect_thingplus_connect_non_ssl(char *mqtt_url, int keepalive)
{
	mock().expectOneCall("mosquitto_connect_callback_set");
	mock().expectOneCall("mosquitto_message_callback_set");
	mock().expectOneCall("mosquitto_disconnect_callback_set");
	mock().expectOneCall("mosquitto_connect_async")
		.withParameter("host", mqtt_url)
		.withParameter("keepalive", keepalive)
		.withParameter("port", 1883);
	mock().expectOneCall("mosquitto_loop_start");
}

void mock_expect_thingplus_connected(char *gw_id)
{
	static char topic_subscribe[256] = {0,};
	static char topic_mqtt_status[256] = {0,};

	memset(topic_subscribe, 0, sizeof(topic_subscribe));
	memset(topic_mqtt_status, 0, sizeof(topic_mqtt_status));

	sprintf(topic_subscribe, "v/a/g/%s/req", gw_id);
	mock().expectOneCall("mosquitto_subscribe")
		.withStringParameter("sub", topic_subscribe)
		.withParameter("qos", 1);

	sprintf(topic_mqtt_status, "v/a/g/%s/mqtt/status", gw_id);
	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", topic_mqtt_status)
		.ignoreOtherParameters();
}

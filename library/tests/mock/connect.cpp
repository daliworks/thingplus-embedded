#include <stdio.h>
#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <CppUTestExt/MockSupport.h>

#include <mock_expect_thingplus.h>

extern "C" {
#include "config.h"
#include "mock_mosquitto.h"
#include "thingplus.h"
}

static char *gw_id;
static char *apikey;
static char *mqtt_url;
static char *restapi_url;
static char *ca_file;
static int keepalive;
static void *t;
static struct thingplus_callback callback;

static void cb_connected(void *cb_arg, enum thingplus_error err)
{
	mock().actualCall("cb_connected")
		.withParameter("cb_arg", cb_arg)
		.withParameter("err", err);
}

static void cb_disconnected(void *cb_arg, enum thingplus_error err)
{
	mock().actualCall("cb_disconnected")
		.withParameter("cb_arg", cb_arg)
		.withParameter("err", err);
}

static void _fixture_setup(void)
{
	gw_id = "012345012345";
	apikey = "D_UoswsKC-v4BHgAk6X4-2i61Zg=";
	mqtt_url = "mqtt.thingplus.net";
	restapi_url = "api.thingplus.net";
	ca_file = CERT_FILE;
	keepalive = 60;

	mock_expect_thingplus_init(gw_id, apikey);
	t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);

	callback.connected = cb_connected,
	callback.disconnected = cb_disconnected,

	thingplus_callback_set(t, &callback, NULL);
}

static void _fixture_teardown(void)
{
	mock_expect_thingplus_cleanup();
	thingplus_cleanup(t);
}

TEST_GROUP(thingplus_connect)
{
	void setup()
	{
		_fixture_setup();
	}

	void teardown()
	{
		_fixture_teardown();
		mock().clear();
	}
};

TEST(thingplus_connect, ssl_using_8883_port_and_necessacy_mosquitto_lib_called)
{
	mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
	int ret = thingplus_connect(t, ca_file, keepalive);

	mock().checkExpectations();
}

TEST(thingplus_connect, invaild_ca_file_return_fail)
{
	ca_file = "invalid.ca_file";
	int ret = thingplus_connect(t, ca_file, keepalive);

	CHECK_EQUAL(-1, ret);
	//mock().checkExpectations();
}

TEST(thingplus_connect, non_ssl_using_1883_port_and_necessacy_mosquitto_lib_called)
{
	mock_expect_thingplus_connect_non_ssl(mqtt_url, keepalive);

	ca_file = NULL;
	thingplus_connect(t, ca_file, keepalive);

	mock().checkExpectations();
}

TEST(thingplus_connect, change_keepalive_to_60_if_user_pass_zero_keepalive)
{
	mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
	keepalive = 0;
	thingplus_connect(t, ca_file, keepalive);

	mock().checkExpectations();
}

TEST(thingplus_connect, cb_connected_called_after_mqtt_connected)
{
	mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
	mock_expect_thingplus_connected(gw_id);
	mock().expectOneCall("cb_connected").ignoreOtherParameters();

	thingplus_connect(t, ca_file, keepalive);
	mock_mosquitto_connected(0);

	mock().checkExpectations();
}

TEST_GROUP(thingplus_disconnect)
{
	void setup()
	{
		_fixture_setup();

		mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
		thingplus_connect(t, ca_file, keepalive);
	}

	void teardown()
	{
		_fixture_teardown();
		mock().clear();
	}
};

TEST(thingplus_disconnect, necessary_mosquitto_lib_called)
{
	mock().expectOneCall("mosquitto_disconnect");
	mock().expectOneCall("mosquitto_loop_stop");

	thingplus_disconnect(t);
	mock().checkExpectations();
}

TEST(thingplus_disconnect, cb_disconnected_called_in_thingplus_disconnected)
{
	mock_expect_thingplus_connected(gw_id);
	mock().expectOneCall("cb_connected").ignoreOtherParameters();
	mock().expectOneCall("cb_disconnected").ignoreOtherParameters();

	mock_mosquitto_connected(0);
	mock_mosquitto_disconnected(0);

	mock().checkExpectations();
}

TEST(thingplus_disconnect, unsubscribe_topic_in_thingplus_disconnect_if_mqtt_is_connected)
{
	char topic_subscribe[256] = {0,};
	sprintf(topic_subscribe, "v/a/g/%s/req", gw_id);
	char topic_mqtt_status[256] = {0,};
	sprintf(topic_mqtt_status, "v/a/g/%s/mqtt/status", gw_id);

	mock_expect_thingplus_connected(gw_id);
	mock().expectOneCall("cb_connected").ignoreOtherParameters();
	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", topic_mqtt_status)
		.ignoreOtherParameters();
	mock().expectOneCall("mosquitto_unsubscribe")
		.withStringParameter("sub", topic_subscribe);
	mock().expectOneCall("mosquitto_disconnect");
	mock().expectOneCall("mosquitto_loop_stop");
	mock().expectOneCall("cb_disconnected").ignoreOtherParameters();

	mock_mosquitto_connected(0);
	thingplus_disconnect(t);
	mock_mosquitto_disconnected(0);

	mock().checkExpectations();
}

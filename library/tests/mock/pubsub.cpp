#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <CppUTestExt/MockSupport.h>

#include <mock_expect_thingplus.h>
#include "fixture.h"

extern "C" {
#include "mock_mosquitto.h"
#include "mqtt/packetize.h"
#include "thingplus.h"
}

static void *t;
static struct thingplus_callback callback;

TEST_GROUP(thingplus_value_publish)
{
	void setup()
	{
		fixture_setup();
		mock_expect_thingplus_init(gw_id, apikey);
		mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
		mock_expect_thingplus_connected(gw_id);

		t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
		thingplus_connect(t, ca_file, keepalive);
		mock_mosquitto_connected(0);
	}

	void teardown()
	{
		mock_expect_thingplus_cleanup();
		thingplus_cleanup(t);
		fixture_teardown();
	}
};

TEST(thingplus_value_publish, single_sensor_single_value_publish_success)
{
	char *payload = "1234567890,16\0";

	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/s/012345012345-temp")
		.withParameter("payload", payload)
		.withParameter("payloadlen", strlen(payload))
		.ignoreOtherParameters();

	struct thingplus_value value = {
		.id = "012345012345-temp",
		.value = "16",
		.time_ms = 1234567890
	};
	int r = thingplus_value_publish(t, 1, &value);

	CHECK_EQUAL(THINGPLUS_ERR_SUCCESS, r);
	mock().checkExpectations();
}

TEST(thingplus_value_publish, single_sensor_two_value_publish_success)
{
	char *temp_id = "012345012345-temp"; 
	char *payload = "1234567890,16,1234567891,17";

	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/s/012345012345-temp")
		.withParameter("payload", payload)
		.withParameter("payloadlen", strlen(payload))
		.ignoreOtherParameters();

	struct thingplus_value value[2];
	value[0].id = temp_id;
	value[0].value = "16";
	value[0].time_ms = 1234567890;
	value[1].id = temp_id;
	value[1].value = "17";
	value[1].time_ms = 1234567891;
	int r = thingplus_value_publish(t, 2, value);

	CHECK_EQUAL(THINGPLUS_ERR_SUCCESS, r);
	mock().checkExpectations();
}

TEST(thingplus_value_publish, single_sensor_two_value_with_null_id_publish_successt)
{
	char *temp_id = "012345012345-temp"; 
	char *payload = "1234567890,16,1234567891,17";

	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/s/012345012345-temp")
		.withParameter("payload", payload)
		.withParameter("payloadlen", strlen(payload))
		.ignoreOtherParameters();

	struct thingplus_value value[3];
	value[0].id = temp_id;
	value[0].value = "16";
	value[0].time_ms = 1234567890;
	value[1].id = NULL;
	value[1].value = "17";
	value[1].time_ms = 1234567891;
	value[2].id = temp_id;
	value[2].value = "17";
	value[2].time_ms = 1234567891;
	int r = thingplus_value_publish(t, 3, value);

	CHECK_EQUAL(THINGPLUS_ERR_SUCCESS, r);
	mock().checkExpectations();
}

TEST(thingplus_value_publish, two_sensor_single_value_publish_success)
{
	char *payload = "{\"012345012345-onoff\": [1234567890,on], \"012345012345-temp\": [1234567891,16]}";

	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345")
		.withParameter("payload", payload)
		.withParameter("payloadlen", strlen(payload))
		.ignoreOtherParameters();

	struct thingplus_value value[2];
	value[0].id = "012345012345-onoff";
	value[0].value = "on";
	value[0].time_ms = 1234567890;
	value[1].id = "012345012345-temp";
	value[1].value = "16";
	value[1].time_ms = 1234567891;
	int r = thingplus_value_publish(t, 2, value);

	CHECK_EQUAL(THINGPLUS_ERR_SUCCESS, r);
	mock().checkExpectations();
}

TEST_GROUP(thingplus_status_publish)
{
	void setup()
	{
		fixture_setup();
		mock_expect_thingplus_init(gw_id, apikey);
		mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
		mock_expect_thingplus_connected(gw_id);

		t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
		thingplus_connect(t, ca_file, keepalive);
		mock_mosquitto_connected(0);
	}

	void teardown()
	{
		mock_expect_thingplus_cleanup();
		thingplus_cleanup(t);
		fixture_teardown();
	}
};

TEST(thingplus_status_publish, single_sensor_status_publish_success)
{
	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/s/012345012345-onoff/status")
		.ignoreOtherParameters();

	struct thingplus_status status;
	status.id = "012345012345-onoff";
	status.timeout_ms = 60 * 1000;
	int r = thingplus_status_publish(t, 1, &status);

	CHECK_EQUAL(0, r);
	mock().checkExpectations();
}

TEST(thingplus_status_publish, missing_id_single_sensor_status_do_not_publish)
{
	struct thingplus_status status;
	status.id = NULL;
	status.timeout_ms = 60 * 1000;
	int r = thingplus_status_publish(t, 1, &status);

	CHECK_EQUAL(0, r);
	mock().checkExpectations();
}

TEST(thingplus_status_publish, nr_thing_is_zero_returns_error)
{
	struct thingplus_status status;
	CHECK_EQUAL(THINGPLUS_ERR_INVALID_ARGUMENT, thingplus_status_publish(t, 0, &status));
}

TEST(thingplus_status_publish, gw_and_sensor_status_publish_success)
{
	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/status")
		.ignoreOtherParameters();

	struct thingplus_status status[2];
	status[0].id = gw_id;
	status[0].timeout_ms = 60 * 1000;
	status[1].id = "012345012345-ononff";
	status[1].timeout_ms = 60 * 1000;
	int r = thingplus_status_publish(t, 2, status);

	CHECK_EQUAL(0, r);
	mock().checkExpectations();
}

TEST(thingplus_status_publish, missing_gw_id_and_sensor_status_publish_success)
{
	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/status")
		.ignoreOtherParameters();

	struct thingplus_status status[3];
	status[0].id = gw_id;
	status[0].timeout_ms = 60 * 1000;
	status[1].id = "012345012345-ononff";
	status[1].timeout_ms = 60 * 1000;
	status[2].id = NULL;
	status[2].timeout_ms = 60 * 1000;
	int r = thingplus_status_publish(t, 3, status);

	CHECK_EQUAL(0, r);
	mock().checkExpectations();
}

TEST(thingplus_status_publish, two_sensor_status_publish_success)
{
	mock().expectNCalls(2, "mosquitto_publish")
		.ignoreOtherParameters();

	struct thingplus_status status[2];
	status[0].id = "012345012345-temp";
	status[0].timeout_ms = 60 * 1000;
	status[1].id = "012345012345-ononff";
	status[1].timeout_ms = 60 * 1000;
	int r = thingplus_status_publish(t, 2, status);

	CHECK_EQUAL(0, r);
	mock().checkExpectations();
}

static bool _actuating(void *cb_arg, const char *id, const char *cmd, const char *options, char *return_message)
{
	mock().actualCall("_actuating")
		.withParameter("id", "012345012345-onoff")
		.withParameter("cmd", "on");

	return true;
}

TEST_GROUP(subscribe)
{
	void setup()
	{
		fixture_setup();
		mock_expect_thingplus_init(gw_id, apikey);
		mock_expect_thingplus_connect_ssl(ca_file, mqtt_url, keepalive);
		mock_expect_thingplus_connected(gw_id);

		t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
		thingplus_connect(t, ca_file, keepalive);
		mock_mosquitto_connected(0);

		callback.actuating = _actuating;
		thingplus_callback_set(t, &callback, NULL);
	}

	void teardown()
	{
		mock_expect_thingplus_cleanup();
		thingplus_cleanup(t);
		fixture_teardown();
	}
};

TEST(subscribe, contorlActuator_message_subscrbed_actuating_callcack_call)
{
	mock().expectOneCall("mosquitto_publish")
		.ignoreOtherParameters();
	mock().expectOneCall("_actuating")
		.withParameter("id", "012345012345-onoff")
		.withParameter("cmd", "on");

	char* payload = NULL;
	int payload_size = 0;
	payload_size = packetize_payload(&payload, payload_size, "{\"id\":\"abcdefg\", \"method\":\"controlActuator\", \"params\": {\"id\":\"0123450123456-onfoff\", \"cmd\":\"on\", \"options\":{\"duration\":3000}}}");

	mock_mosquitto_subscribed(NULL, payload, payload_size);
	mock().checkExpectations();

	packetize_free(payload);
}

TEST(subscribe, no_method_json_subscribed_publish_error)
{
	mock().expectOneCall("mosquitto_publish")
		.withParameter("topic", "v/a/g/012345012345/res")
		.ignoreOtherParameters();

	char* payload = NULL;
	int payload_size = 0;
	payload_size = packetize_payload(&payload, payload_size, "{\"id\":\"abcdefg\"}");

	mock_mosquitto_subscribed(NULL, payload, payload_size);
	mock().checkExpectations();

	packetize_free(payload);
}

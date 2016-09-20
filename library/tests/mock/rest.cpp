#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <CppUTestExt/MockSupport.h>

#include <mock_expect_thingplus.h>
#include <mock_expect_tcurl.h>

extern "C" {
#include "config.h"
#include "thingplus_types.h"
#include "mock_curl.h"
#include "rest/rest.h"
#include "thingplus.h"
}

static char *gw_id;
static char *apikey;
static char *mqtt_url;
static char *restapi_url;
static char *ca_file;
static int keepalive;
static void *t;
static char *device_info_json = "{\"name\":\"DEVNAME\", \"model\":\"jsonrpcFullV1.0\",\"owner\":\"OWNER\",\"mtime\":\"123\",\"ctime\":\"456\",\"id\":\"dev_id\"}";
static char *sensor_info_json = "{\"id\":\"ID\", \"network\":\"NETWORK\",\"driverName\":\"DRIVER_NAME\",\"mtime\":\"123\",\"ctime\":\"456\",\"model\":\"MODEL\",\"type\":\"TYPE\",\"category\":\"CATEGORY\",\"name\":\"NAME\",\"device_id\":\"DEVICE_ID\",\"owner\":\"OWNER\"}";

static void fixture_setup()
{
	gw_id = "012345012345";
	apikey = "D_UoswsKC-v4BHgAk6X4-2i61Zg=";
	mqtt_url = "mqtt.thingplus.net";
	restapi_url = "api.thingplus.net";
	ca_file = CERT_FILE;
	keepalive = 60;

	mock_expect_tcurl_init();
	t = rest_init(gw_id, apikey, restapi_url);
}

static void fixture_teardown()
{
	mock_expect_tcurl_cleanup();
	rest_cleanup(t);
}

TEST_GROUP(rest)
{
	void setup()
	{
		fixture_setup();
	}

	void teardown()
	{
		fixture_teardown();
		mock().clear();
	}
};

TEST(rest, rest_deviceinfo_success)
{
	mock_expect_tcurl_read();

	char *dev_id = "abcdefg";
	struct thingplus_device device;
	memset(&device, 0, sizeof(device));

	mock_curl_payload_set(device_info_json);
	int ret = rest_deviceinfo(t, dev_id, &device);

	CHECK_EQUAL(0, ret);
	STRCMP_EQUAL("DEVNAME", device.name);
	STRCMP_EQUAL("jsonrpcFullV1.0", device.model);
	STRCMP_EQUAL("OWNER", device.owner);
	CHECK_EQUAL(123, device.mtime);
	CHECK_EQUAL(456, device.ctime);
	STRCMP_EQUAL("dev_id", device.id);
	mock().checkExpectations();
}

TEST(rest, rest_deviceinfo_err_tcurl_read_returns_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)NULL);

	char *dev_id = "abcdefg";
	struct thingplus_device device;
	memset(&device, 0, sizeof(device));

	mock_curl_payload_set(device_info_json);
	int ret = rest_deviceinfo(t, dev_id, &device);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();
}

TEST(rest, rest_sensorinfo_success)
{
	mock_expect_tcurl_read();

	char *sensor_id = "dummy_sensor_id";
	struct thingplus_sensor sensor;
	mock_curl_payload_set(sensor_info_json);
	int ret = rest_sensorinfo(t, sensor_id, &sensor);

	CHECK_EQUAL(0, ret);
	STRCMP_EQUAL("ID", sensor.id);
	STRCMP_EQUAL("NETWORK", sensor.network);
	STRCMP_EQUAL("DRIVER_NAME", sensor.driver_name);
	STRCMP_EQUAL("MODEL", sensor.model);
	STRCMP_EQUAL("TYPE", sensor.type);
	STRCMP_EQUAL("CATEGORY", sensor.category);
	STRCMP_EQUAL("NAME", sensor.name);
	STRCMP_EQUAL("DEVICE_ID", sensor.device_id);
	STRCMP_EQUAL("OWNER", sensor.owner);
	CHECK_EQUAL(123, sensor.mtime);
	CHECK_EQUAL(456, sensor.ctime);
	mock().checkExpectations();
}

TEST(rest, rest_sensorinfo_err_tcurl_read_return_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)NULL);

	char *sensor_id = "dummy_sensor_id";
	struct thingplus_sensor sensor;
	int ret = rest_sensorinfo(t, sensor_id, &sensor);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();
}

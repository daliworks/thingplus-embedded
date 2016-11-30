#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <CppUTestExt/MockSupport.h>

#include <mock_expect_thingplus.h>
#include <mock_expect_tcurl.h>
#include "fixture.h"

extern "C" {
#include "thingplus_types.h"
#include "mock_curl.h"
#include "rest/rest.h"
#include "thingplus.h"
}

static void *t;

TEST_GROUP(rest_xxxinfo)
{
	void setup()
	{
		fixture_setup();

		mock_expect_tcurl_init();
		t = rest_init(gw_id, apikey, restapi_url);
	}

	void teardown()
	{
		mock_expect_tcurl_cleanup();
		rest_cleanup(t);
		t = NULL;

		fixture_teardown();
	}
};

TEST(rest_xxxinfo, rest_deviceinfo_success)
{
	mock_expect_tcurl_read();
	struct thingplus_device device;
	memset(&device, 0, sizeof(device));
	int ret = rest_deviceinfo(t, "dev_id", &device);

	CHECK_EQUAL(0, ret);
	STRCMP_EQUAL("DEVNAME", device.name);
	STRCMP_EQUAL("jsonrpcFullV1.0", device.model);
	STRCMP_EQUAL("OWNER", device.owner);
	CHECK_EQUAL(123, device.mtime);
	CHECK_EQUAL(456, device.ctime);
	STRCMP_EQUAL("dev_id", device.id);
	mock().checkExpectations();

}

TEST(rest_xxxinfo, rest_deviceinfo_err_tcurl_read_returns_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)NULL);

	struct thingplus_device device;
	int ret = rest_deviceinfo(t, "dev_id", &device);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();
}

TEST(rest_xxxinfo, rest_sensorinfo_success)
{
	mock_expect_tcurl_read();

	struct thingplus_sensor sensor;
	int ret = rest_sensorinfo(t, "sensor_id", &sensor);

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

TEST(rest_xxxinfo, rest_sensorinfo_err_tcurl_read_return_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)NULL);

	struct thingplus_sensor sensor;
	int ret = rest_sensorinfo(t, "sensor_id", &sensor);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();
}

TEST(rest_xxxinfo, rest_gatewayinfo_success)
{
	mock_expect_tcurl_read();

	struct thingplus_gateway gateway;
	int ret = rest_gatewayinfo(t, &gateway);

	CHECK_EQUAL(0, ret);
	CHECK_EQUAL(538, gateway.site);
	CHECK_EQUAL(123, gateway.mtime);
	CHECK_EQUAL(456, gateway.ctime);
	CHECK_EQUAL(34, gateway.model);
	CHECK(gateway.discoverable);
	STRCMP_EQUAL("NAME", gateway.name);
	STRCMP_EQUAL("ID", gateway.id);
	CHECK_EQUAL(2, gateway.nr_devices);
	STRCMP_EQUAL("c", gateway.devices[0]);
	STRCMP_EQUAL("d", gateway.devices[1]);
	CHECK_EQUAL(2, gateway.nr_sensors);
	STRCMP_EQUAL("a", gateway.sensors[0]);
	STRCMP_EQUAL("b", gateway.sensors[1]);
	mock().checkExpectations();
}

TEST(rest_xxxinfo, rest_gatewayinfo_free_success)
{
	mock().ignoreOtherCalls();

	struct thingplus_gateway gateway;
	rest_gatewayinfo(t, &gateway);

	rest_gatewayinfo_free(&gateway);
}

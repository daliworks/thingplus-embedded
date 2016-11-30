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
#include "restapi_server.h"
#include "thingplus_types.h"
#include "mock_curl.h"
#include "rest/rest.h"
#include "thingplus.h"
}

static void *t;

TEST_GROUP(rest_register)
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
		fixture_teardown();
		t = NULL;
	}
};

TEST(rest_register, device_resigster_success)
{
	mock_expect_tcurl_read();

	mock_expect_tcurl_read();
	mock_expect_tcurl_post();

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(0, ret);
	mock().checkExpectations();

}

TEST(rest_register, device_register_name_null_return_fail)
{
	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, NULL, 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(-1, ret);
}

TEST(rest_register, device_register_no_gateway_null_return_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)NULL);

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(-1, ret);
}

TEST(rest_register, device_register_gateway_not_discoverable_return_fail)
{
	char *gateway_info_json = "{\
				   \"autoCreateDiscoverable\":\"n\", \
				   }";

	restapi_server_response_set("https://api.thingplus.net/gateways/012345012345", gateway_info_json);

	mock_expect_tcurl_read();

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();

	restapi_server_reset();
}

TEST(rest_register, device_register_gatewaymodel_no_devicemodel_return_fail)
{
	char *gateway_info_json = "{\
				   \"autoCreateDiscoverable\":\"y\", \
				   \"id\":\"ID\"}";
	restapi_server_response_set("https://api.thingplus.net/gateways/012345012345", gateway_info_json);


	mock_expect_tcurl_read();
	mock_expect_tcurl_read();

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();

	restapi_server_reset();
}

TEST(rest_register, sensor_register_sensor_id_null_return_fail)
{
	int ret = rest_sensor_register(t, "NAME", 0, "TYPE", "DEVICE_ID", NULL);
	CHECK_EQUAL(-1, ret);

}

TEST(rest_register, register_sensor_to_unregister_gateway_return_fail)
{
	mock().ignoreOtherCalls();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_sensor_register(t, "NAME", 0, "TYPE", "DEVICE_ID", sensor_id);

	CHECK_EQUAL(-1, ret);
	mock().clear();
}

TEST(rest_register, sensor_register_get_gatewayinfo)
{
	struct rest {
		char *gw_id;
		char *apikey;
		void* curl;

		struct thingplus_gateway gateway_info;
	};

	mock_expect_tcurl_read();
	mock_expect_tcurl_read();
	mock_expect_tcurl_read();
	mock_expect_tcurl_read();
	mock_expect_tcurl_post();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	rest_sensor_register(t, "NAME", 0, "TYPE", "DEVICE_ID", sensor_id);
	struct rest *r = (struct rest *)t;

	CHECK_EQUAL(34, r->gateway_info.model);
}

TEST(rest_register, register_sensor_to_not_discoverable_gateway_return_fail)
{
	mock().ignoreOtherCalls();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_sensor_register(t, "NAME", 0, "TYPE", "DEVICE_ID", sensor_id);
	CHECK_EQUAL(-1, ret);
}

TEST(rest_register, sensor_register_success)
{
	mock_expect_tcurl_read();

	mock_expect_tcurl_read();

	mock_expect_tcurl_read();

	mock_expect_tcurl_read();
	mock_expect_tcurl_post();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_sensor_register(t, "NAME", 0, "number", "jsonrpcFullV1.0", sensor_id);

	CHECK_EQUAL(0, ret);
	mock().checkExpectations();
}

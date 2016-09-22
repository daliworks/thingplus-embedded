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
	char *gateway_info_json = "{\
				   \"sensors\":[\"a\",\"b\"], \
				   \"devices\":[\"c\",\"d\"], \
				   \"_site\":\"538\",\
				   \"autoCreateDiscoverable\":\"y\", \
				   \"deviceModels\":[{\"model\":\"jsonrpcFullV1.0\"}], \
				   \"name\":\"NAME\", \
				   \"model\":\"34\", \
				   \"mtime\":\"123\", \
				   \"ctime\":\"456\", \
				   \"id\":\"ID\"}";
	mock_curl_payload_append(gateway_info_json);
	mock_expect_tcurl_read();

	char *gateway_model_json = "{\
				    \"deviceModels\":[{\
					\"id\":\"jsonrpcFullV1.0\", \
	     				\"discoverable\": \"y\", \
					\"idTemplate\": \"{gatewayId}-{deviceAddress}\",\
					\"sensors\": [{\
						\"network\":\"jsonrpc\",\
	     					\"driverName\":\"jsonrpcSensor\",\
						\"model\":\"jsonrpcNumber\",\
						\"type\":\"number\",\
						\"category\":\"sensor\"}]\
					}]\
				}";	
	mock_curl_payload_append(gateway_model_json);
	mock_expect_tcurl_read();

	mock_curl_payload_append("{\"server_response\":\"dummy\"}");
	mock_expect_tcurl_post();

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(0, ret);
	mock().checkExpectations();

	mock_curl_payload_clear();
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
	mock_curl_payload_append(gateway_info_json);
	mock_expect_tcurl_read();

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();

	mock_curl_payload_clear();
}

TEST(rest_register, device_register_gatewaymodel_no_devicemodel_return_fail)
{
	char *gateway_info_json = "{\
				   \"autoCreateDiscoverable\":\"y\", \
				   \"id\":\"ID\"}";
	mock_curl_payload_append(gateway_info_json);
	mock_expect_tcurl_read();
	mock_expect_tcurl_read();

	char device_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_device_register(t, "name", 0, "jsonrpcFullV1.0", device_id);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();

	mock_curl_payload_clear();
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

	char *gateway_info_json = "{\
				   \"sensors\":[\"a\",\"b\"], \
				   \"devices\":[\"c\",\"d\"], \
				   \"_site\":\"538\",\
				   \"autoCreateDiscoverable\":\"n\", \
				   \"deviceModels\":[{\"model\":\"jsonrpcFullV1.0\"}], \
				   \"name\":\"NAME\", \
				   \"model\":\"34\", \
				   \"mtime\":\"123\", \
				   \"ctime\":\"456\", \
				   \"id\":\"ID\"}";
	mock_curl_payload_append(gateway_info_json);
	mock_expect_tcurl_read();
	mock().ignoreOtherCalls();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	rest_sensor_register(t, "NAME", 0, "TYPE", "DEVICE_ID", sensor_id);
	struct rest *r = (struct rest *)t;

	CHECK_EQUAL(34, r->gateway_info.model);
}

TEST(rest_register, register_sensor_to_not_discoverable_gateway_return_fail)
{
	char *gateway_info_json = "{\
				   \"autoCreateDiscoverable\":\"y\", \
				   \"id\":\"ID\"}";
	mock().ignoreOtherCalls();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_sensor_register(t, "NAME", 0, "TYPE", "DEVICE_ID", sensor_id);
	CHECK_EQUAL(-1, ret);
	mock_curl_payload_clear();
}

TEST(rest_register, sensor_register_success)
{
	char *gateway_info_json = "{\
				   \"sensors\":[\"a\",\"b\"], \
				   \"devices\":[\"c\",\"d\"], \
				   \"_site\":\"538\",\
				   \"autoCreateDiscoverable\":\"y\", \
				   \"deviceModels\":[{\"model\":\"jsonrpcFullV1.0\"}], \
				   \"name\":\"NAME\", \
				   \"model\":\"34\", \
				   \"mtime\":\"123\", \
				   \"ctime\":\"456\", \
				   \"id\":\"ID\"}";
	mock_curl_payload_append(gateway_info_json);
	mock_expect_tcurl_read();

	char *gateway_model_json = "{\
				    \"deviceModels\":[{\
					\"id\":\"jsonrpcFullV1.0\", \
	     				\"discoverable\": \"y\", \
					\"idTemplate\": \"{gatewayId}-{deviceAddress}\",\
					\"sensors\": [{\
						\"network\":\"jsonrpc\",\
	     					\"driverName\":\"jsonrpcSensor\",\
						\"model\":\"jsonrpcNumber\",\
						\"type\":\"number\",\
						\"category\":\"sensor\"}]\
					}]\
				}";	
	mock_curl_payload_append(gateway_model_json);
	mock_expect_tcurl_read();

	char *device_model_json = "{\
				   	\"model\": \"jsonrpcFullV1.0\", \
	     				\"discoverable\": \"y\", \
				}";	
	mock_curl_payload_append(device_model_json);
	mock_expect_tcurl_read();

	mock_curl_payload_append("{\"idTemplate\":\"{gatewayId}-{deviceAddress}\"}");
	mock_expect_tcurl_read();
	mock_curl_payload_append("{\"server_response\":\"dummy\"}");
	mock_expect_tcurl_post();

	char sensor_id[THINGPLUS_ID_LENGTH] = {0,};
	int ret = rest_sensor_register(t, "NAME", 0, "number", "jsonrpcFullV1.0", sensor_id);

	CHECK_EQUAL(0, ret);
	mock().checkExpectations();
	mock_curl_payload_clear();
}

#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>

extern "C" {
#include "../restapi_server.h"
}
char *url = NULL;
char *expect_response = NULL;
TEST_GROUP(restapi_server)
{
	void setup()
	{
		url = "https://api.thingplus.net/gateways/012345012345";
		expect_response = "{\
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
	}

	void teardown()
	{
	}
};

TEST(restapi_server, get_expect_reponse)
{

	char *response = restapi_server_expect_get(url);
	STRCMP_EQUAL(expect_response, response);
}

TEST(restapi_server, restapi_server_call_return_default_value)
{
	char *response = restapi_server_call(url);
	STRCMP_EQUAL(expect_response, response);
}

TEST(restapi_server, change_response)
{
	restapi_server_response_set(url, "123");

	char *response = restapi_server_call(url);
	STRCMP_EQUAL("123", response);

	restapi_server_reset();
}

TEST(restapi_server, after_reset_response_default_value)
{
	restapi_server_response_set(url, "123");
	restapi_server_reset();

	char *response = restapi_server_call(url);
	STRCMP_EQUAL(expect_response, response);
}

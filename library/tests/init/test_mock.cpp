#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <CppUTestExt/MockSupport.h>

#include "mock_expect_thingplus.h"

extern "C" {
#include "config.h"
#include "thingplus.h"
}

static char *gw_id;
static char *apikey;
static char *mqtt_url;
static char *restapi_url;

TEST_GROUP(init)
{
	void setup()
	{
		gw_id = "012345012345";
		apikey = "D_UoswsKC-v4BHgAk6X4-2i61Zg=";
		mqtt_url = "mqtt.thingplus.net";
		restapi_url = "api.thingplus.net";
	}

	void teardown()
	{
		mock().clear();
	}
};

TEST(init, init_and_cleanup_success)
{
	mock_expect_thingplus_init(gw_id, apikey);
	mock_expect_thingplus_cleanup();

	void *t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
	CHECK(t);

	thingplus_cleanup(t);
	mock().checkExpectations();
}

TEST(init, if_mosquitto_new_failed_thignplus_init_returns_null)
{
	mock().expectOneCall("mosquitto_lib_init");
	mock().expectOneCall("mosquitto_new")
		.withParameter("id", gw_id)
		.andReturnValue((void*)NULL);
	mock().expectOneCall("mosquitto_lib_cleanup");

	void *t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
	CHECK(!t);

	thingplus_cleanup(t);

	mock().checkExpectations();
}

TEST(init, if_gw_id_is_null_thingplus_init_returns_null)
{
	gw_id = NULL;

	void *t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
	CHECK(!t);
}

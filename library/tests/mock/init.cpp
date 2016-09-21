#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <CppUTestExt/MockSupport.h>

#include "mock_expect_thingplus.h"
#include "fixture.h"

extern "C" {
#include "thingplus.h"
}

TEST_GROUP(init)
{
	void setup()
	{
		fixture_setup();
	}

	void teardown()
	{
		fixture_teardown();
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

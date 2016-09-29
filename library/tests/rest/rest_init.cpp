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

TEST_GROUP(rest_init_cleanup)
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

TEST(rest_init_cleanup, init_success)
{
	mock_expect_tcurl_init();
	mock_expect_tcurl_cleanup();

	t = rest_init(gw_id, apikey, restapi_url);

	CHECK(t);

	rest_cleanup(t);

	mock().checkExpectations();
}

TEST(rest_init_cleanup, init_apikey_null_return_fail)
{
	t = rest_init(gw_id, NULL, restapi_url);

	CHECK_FALSE(t);
}

TEST(rest_init_cleanup, rest_cleanup_with_null_instance_no_segmentfault)
{
	rest_cleanup(NULL);
}



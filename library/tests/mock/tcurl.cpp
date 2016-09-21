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
#include "mock_curl.h"
#include "rest/tcurl.h"
#include "rest/url.h"
#include "thingplus.h"
}

static char *gw_id;
static char *apikey;
static char *mqtt_url;
static char *restapi_url;
static char *ca_file;
static int keepalive;

static void fixture_setup()
{
	gw_id = "012345012345";
	apikey = "D_UoswsKC-v4BHgAk6X4-2i61Zg=";
	mqtt_url = "mqtt.thingplus.net";
	restapi_url = "api.thingplus.net";
	ca_file = CERT_FILE;
	keepalive = 60;

}

static void fixture_teardown()
{
}

TEST_GROUP(tcurl)
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

TEST(tcurl, init_success)
{
	mock().expectOneCall("curl_global_init");
	mock().expectOneCall("curl_slist_append")
		.withParameter("ptr", "username:012345012345")
		.andReturnValue((void*)0xdeadbeef);
	mock().expectOneCall("curl_slist_append")
		.withParameter("ptr", "apikey:D_UoswsKC-v4BHgAk6X4-2i61Zg=")
		.andReturnValue((void*)0xdeadbeef);
	mock().expectOneCall("curl_slist_append")
		.withParameter("ptr", "Content-Type: application/json")
		.andReturnValue((void*)0xdeadbeef);

	void *tcurl = tcurl_init(gw_id, apikey);

	CHECK(tcurl);
	mock().checkExpectations();
}


TEST(tcurl, gw_id_is_null_return_fail)
{
	gw_id = NULL;
	void *tcurl = tcurl_init(gw_id, apikey);

	CHECK_FALSE(tcurl);
}

TEST(tcurl, apikey_is_null_return_fail)
{
	apikey = NULL;
	void* tcurl = tcurl_init(gw_id, apikey);

	CHECK_FALSE(tcurl);
}

TEST(tcurl, cleanup_success)
{
	mock_expect_tcurl_init();
	mock().expectOneCall("curl_global_cleanup");
	mock().expectOneCall("curl_slist_free_all");

	void *tcurl = tcurl_init(gw_id, apikey);
	tcurl_cleanup(tcurl);

	mock().checkExpectations();
}

TEST(tcurl, intance_is_null_cleanup_do_not_segmentfault)
{
	tcurl_cleanup(NULL);
}

TEST_GROUP(tcurl_read_post)
{
	void *tcurl;
	void setup()
	{
		fixture_setup();
		mock_expect_tcurl_init();

		tcurl = tcurl_init(gw_id, apikey);
	}

	void teardown()
	{
		mock_expect_tcurl_cleanup();
		tcurl_cleanup(tcurl);

		fixture_teardown();

		mock().clear();
	}
};

TEST(tcurl_read_post, read)
{
	mock_expect_tcurl_read();

	struct tcurl_payload p;
	char *device_id = "abcdefg";
	char *url = url_get(URL_INDEX_DEVICE_INFO, gw_id, device_id);

	mock_curl_payload_set("{\"name\":\"a\"}");

	int ret  = tcurl_read(url, tcurl, &p);

	CHECK_FALSE(ret);
	mock().checkExpectations();

	tcurl_payload_free(&p);
}

TEST(tcurl_read_post, read_with_err_curl_easy_perform_returns_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)0xdeadbeef);
	mock().expectNCalls(4, "curl_easy_setopt");
	mock().expectOneCall("curl_easy_perform")
		.andReturnValue(1);
	mock().expectOneCall("curl_easy_cleanup");

	struct tcurl_payload p;
	char *device_id = "abcdefg";
	char *url = url_get(URL_INDEX_DEVICE_INFO, gw_id, device_id);
	mock_curl_payload_set("{\"name\":\"a\"}");

	int ret  = tcurl_read(url, tcurl, &p);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();

	tcurl_payload_free(&p);
}

TEST(tcurl_read_post, read_with_err_curl_easy_init_returns_fail)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)NULL);

	struct tcurl_payload p;
	char *device_id = "abcdefg";
	char *url = url_get(URL_INDEX_DEVICE_INFO, gw_id, device_id);
	int ret  = tcurl_read(url, tcurl, &p);

	CHECK_EQUAL(-1, ret);
	mock().checkExpectations();
}

TEST(tcurl_read_post, post)
{
	mock_expect_tcurl_post();

	struct tcurl_payload p;
	char *device_id = "abcdefg";
	char *url = url_get(URL_INDEX_DEVICE_INFO, gw_id, device_id);

	mock_curl_payload_set("{\"name\":\"a\"}");

	int ret  = tcurl_post(url, tcurl, (void*)"abcdefg", &p);

	CHECK_FALSE(ret);
	mock().checkExpectations();

	tcurl_payload_free(&p);
}

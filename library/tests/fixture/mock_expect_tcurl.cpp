#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

void mock_expect_tcurl_init(void)
{
	mock().expectOneCall("curl_global_init");
	mock().expectNCalls(3, "curl_slist_append")
		.ignoreOtherParameters()
		.andReturnValue((void*)0xdeadbeef);
}

void mock_expect_tcurl_cleanup(void)
{
	mock().expectOneCall("curl_slist_free_all");
	mock().expectOneCall("curl_global_cleanup");
}

void mock_expect_tcurl_read(void)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)0xdeadbeef);
	mock().expectNCalls(4, "curl_easy_setopt");
	mock().expectOneCall("curl_easy_perform")
		.andReturnValue(0);
	mock().expectOneCall("curl_easy_cleanup");
}

void mock_expect_tcurl_post(void)
{
	mock().expectOneCall("curl_easy_init")
		.andReturnValue((void*)0xdeadbeef);
	mock().expectNCalls(5, "curl_easy_setopt");
	mock().expectOneCall("curl_easy_perform")
		.andReturnValue(0);
	mock().expectOneCall("curl_easy_cleanup");
}

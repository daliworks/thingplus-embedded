#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <CppUTestExt/MockSupport_c.h>


struct curl_slist;

typedef enum {
	CURLOPT_WRITEDATA = 10001,
	CURLOPT_URL = 10002,
	CURLOPT_WRITEFUNCTION = 20011,
	CURLOPT_POSTFIELDS = 15,
	CURLOPT_HTTPHEADER = 10023,
} CURLoption;

typedef void CURL;

typedef size_t (*curl_write_callback)(char *buffer,
		size_t size,
		size_t nitems,
		void *outstream);

static curl_write_callback _write_callback;

static char *_contents;
static void *_tcurl_payload;

void _mock_curl_payload_set(char *contents)
{
	_contents = contents;
}

void mock_curl_payload_set(char *contents, void *tcurl_payload)
{
	_contents = contents;
	_tcurl_payload  = tcurl_payload;
}

void *curl_easy_init(void)
{
	mock_c()->actualCall("curl_easy_init");

	return mock_c()->returnValue().value.pointerValue;
}

int curl_easy_setopt(CURL *curl, CURLoption option, ...)
{
	printf("%d\n", option);
	printf("%d\n", option);
	printf("%d\n", option);
	printf("%d\n", option);
	printf("%d\n", option);

	va_list args;

	va_start(args, option);
	switch (option) {
		case CURLOPT_WRITEFUNCTION:
			_write_callback = va_arg(args, curl_write_callback);
			break;
		case CURLOPT_WRITEDATA:
			_tcurl_payload = va_arg(args, void *);
			break;
	}

	va_end(args);

	mock_c()->actualCall("curl_easy_setopt");
	return 0;
}

int curl_easy_perform(CURL *curl)
{
	if (_write_callback)
		_write_callback(_contents, strlen(_contents), 1, _tcurl_payload);

	mock_c()->actualCall("curl_easy_perform");

	return mock_c()->returnValue().value.intValue;
}

void curl_easy_cleanup(CURL *curl)
{
	mock_c()->actualCall("curl_easy_cleanup");
}

void curl_global_cleanup(void)
{
	mock_c()->actualCall("curl_global_cleanup");
}

void curl_slist_free_all(struct curl_slist * s)
{
	mock_c()->actualCall("curl_slist_free_all");
}

void curl_global_init(long flags)
{
	mock_c()->actualCall("curl_global_init");
}

struct curl_slist *curl_slist_append(struct curl_slist *s, const char *ptr)
{
	mock_c()->actualCall("curl_slist_append")
		->withStringParameters("ptr", ptr);

	return mock_c()->returnValue().value.pointerValue;
}

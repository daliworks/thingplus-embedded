#include "CppUTest/TestHarness.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include <unistd.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "thingplus.h"
}

void *t = NULL;
char *gw_id;
char *apikey;
char *mqtt_url;
char *restapi_url;
char *ca_file;
int keepalive;

TEST_GROUP(CONNECT) {
	void setup() 
	{
		gw_id = "012345012345";
		apikey = "D_UoswsKC-v4BHgAk6X4-2i61Zg=";
		mqtt_url = "dmqtt.thingplus.net";
		restapi_url = "api.thingplus.net";
		ca_file = CERT_FILE;
		keepalive = 60;

		t = thingplus_init(gw_id, apikey, mqtt_url, restapi_url);
	}

	void teardown() 
	{
		thingplus_disconnect(t);
		thingplus_cleanup(t);
		t = NULL;
	}
};


TEST(CONNECT, CA_FILE_NULL_INIT_SUCCESS)
{
	ca_file = NULL;
	CHECK_EQUAL(0, thingplus_connect(t, ca_file, keepalive));
}

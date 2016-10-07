#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include "fixture/async.h"

extern "C" {
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "../src/gateway.c"
}

static char *gw_id;
static char *apikey;
static char *ca_cert;
static char *sensor_id;

TEST_GROUP(GATEWAY) {
	void setup() {
		gw_id =  "a820664d5df4";
		apikey = "7VYHKL-YKVp278d7zKOAtVOqruA=";
		ca_cert = "../../cert/ca-cert.pem";
		sensor_id = "countEvent-a820664d5df4";
	}

	void teardown() {
		gw_disconnect();
		gw_cleanup();
	}
};

bool _count_event_sensor_on(void)
{
	return true;
}

char* _count_event_sensor_get(void)
{
	return "50";
}

TEST(GATEWAY, GatewayInit)
{
	struct sensor s;
	bzero(&s, sizeof(s));

	strcpy(s.id, sensor_id);
	s.ops.on = _count_event_sensor_on;
	s.ops.get = _count_event_sensor_get;

	int ret = gw_init(gw_id, apikey, 1, &s);

	CHECK_EQUAL(0, ret);
}

TEST(GATEWAY, GatewayCleanupNoSegmentFalut)
{
	gw_init(gw_id, apikey, 0, NULL);
	gw_cleanup();
}

TEST(GATEWAY, GatewayInitFailedGivenInvalidGWID)
{
	// given
	gw_id = NULL;

	// when
	int ret = gw_init(gw_id, apikey, 0, NULL);

	// then
	CHECK_EQUAL(-1, ret);
}

TEST(GATEWAY, GatewayConnectSuccess)
{
	gw_init(gw_id, apikey, 0, NULL);

	// when
	int ret = gw_connect(ca_cert);

	// then
	CHECK_EQUAL(0, ret);
}


TEST(GATEWAY, GatewayConnectCallTwice)
{
	gw_init(gw_id, apikey, 0, NULL);

	// given 
	gw_connect(ca_cert);

	// when 
	int ret = gw_connect(ca_cert);

	// then
	CHECK_EQUAL(1, ret);
}

TEST(GATEWAY, SendEventSensorValue)
{
	gw_init(gw_id, apikey, 0, NULL);

	// given 
	gw_connect(ca_cert);

	// when
	int ret = gw_event_value_publish("countEvent-a820664d5df4", "100");

	// then
	CHECK_EQUAL(0, ret);
}

TEST(GATEWAY, SendEventSensorValueNotConnectedGateway)
{
	// when
	int ret = gw_event_value_publish("countEvent-a820664d5df4", "100");

	// then
	CHECK_EQUAL(-1, ret);
}

TEST(GATEWAY, StatsPublish)
{
	struct sensor s;
	bzero(&s, sizeof(s));

	strcpy(s.id, sensor_id);
	s.ops.on = _count_event_sensor_on;
	s.ops.get = _count_event_sensor_get;

	int ret = gw_init(gw_id, apikey, 1, &s);
	gw_connect(ca_cert);

	union sigval sigval;
	sigval.sival_ptr = gw;
	_status_publish(sigval);
}

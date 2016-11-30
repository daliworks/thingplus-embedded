#ifndef _FIXTURE_H_
#define _FIXTURE_H_

static char *gw_id;
static char *apikey;
static char *mqtt_url;
static char *restapi_url;
static char *ca_file;
static int keepalive;

static void fixture_setup()
{
	gw_id = "012345012345";
	apikey = "D_UoswsKC-v5BHgAk6X4-2i61Zg=";
	mqtt_url = "mqtt.thingplus.net";
	restapi_url = "api.thingplus.net";
	ca_file = "../cert/ca-cert.pem";
	keepalive = 60;
}

static void fixture_teardown()
{
	mock().clear();
}

#endif //#ifndef _FIXTURE_H_

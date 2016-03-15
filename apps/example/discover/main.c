#include <stdio.h>

#include "tube_http.h"

int main(int argc, char **argv)
{
	char *gwid = "84eb18afeb0c";
	char *apikey = "YHZbDd0n6TQpPJP2MLBKHM7hn7o=";

	void *t = tube_http_init(gwid, apikey);

	char device_id[TUBE_HTTP_MAX_ID];
	tube_http_device_register(t, "restapi_test", 1, "jsonrpcFullV1.0", device_id);

	char sensor_id[TUBE_HTTP_MAX_ID];
	tube_http_sensor_register(t, "test", 10000, "temperature", "84eb18afeb0c-0", sensor_id);

	tube_http_cleanup(t);

	return 0;
}



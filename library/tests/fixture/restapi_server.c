#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct responses{
	char *url;
	char *response;
} defaults[] = {
	{	/*URL_INDEX_GATEWAY_INFO*/
		.url = "https://api.thingplus.net/gateways/012345012345",
		.response = "{\
			     \"sensors\":[\"a\",\"b\"], \
			     \"devices\":[\"c\",\"d\"], \
			     \"_site\":\"538\",\
			     \"autoCreateDiscoverable\":\"y\", \
			     \"deviceModels\":[{\"model\":\"jsonrpcFullV1.0\"}], \
			     \"name\":\"NAME\", \
			     \"model\":\"34\", \
			     \"mtime\":\"123\", \
			     \"ctime\":\"456\", \
			     \"id\":\"ID\"}"
	},
	{	/* URL_INDEX_DEVICE_INFO */
		.url = "https://api.thingplus.net/gateways/012345012345/devices/jsonrpcFullV1.0",
		.response = "{\
			     \"model\": \"jsonrpcFullV1.0\", \
			     \"discoverable\": \"y\", \
			     }"	
	},
	{	/* URL_INDEX_SENSOR_INFO */
		.url = "https://api.thingplus.net/gateways/012345012345/sensors/sensor_id",
		.response = "{\
			     \"id\":\"ID\", \
			     \"network\":\"NETWORK\", \
			     \"driverName\":\"DRIVER_NAME\", \
			     \"mtime\":\"123\", \
			     \"ctime\":\"456\", \
			     \"model\":\"MODEL\", \
			     \"type\":\"TYPE\", \
			     \"category\":\"CATEGORY\", \
			     \"name\":\"NAME\", \
			     \"device_id\":\"DEVICE_ID\", \
			     \"owner\":\"OWNER\"}"
	},
	{	/* URL_INDEX_GATEWAY_MODEL */
		.url = "https://api.thingplus.net/gatewayModels/34",
		.response = "{\
			     \"deviceModels\":[{\
			     	\"id\":\"jsonrpcFullV1.0\", \
			     	\"discoverable\": \"y\", \
			     	\"idTemplate\": \"{gatewayId}-{deviceAddress}\",\
			     	\"sensors\": [{\
			     		\"network\":\"jsonrpc\",\
			     		\"driverName\":\"jsonrpcSensor\",\
			     		\"model\":\"jsonrpcNumber\",\
			     		\"type\":\"number\",\
			     		\"category\":\"sensor\"}]\
				}]\
			     }"
	},
	{	/* URL_INDEX_DEVICE_REGISTER */
		.url = "https://api.thingplus.net/gateways/012345012345/devices",
		.response = "{\"server_response\":\"dummy\"}"
	},
	{	/* URL_INDEX_SENSOR_REGISTER */
		.url = "https://api.thingplus.net/gateways/012345012345/sensors",
		.response = "{\"server_response\":\"dummy\"}"
	},
	{	/* URL_INDEX_SENSOR_DRIVERS */
		.url = "https://api.thingplus.net/sensorDrivers/?filter[id]=jsonrpcSensor",
		.response = "{\"idTemplate\":\"{gatewayId}-{deviceAddress}\"}"
	},



	{
		.url = "https://api.thingplus.net/gateways/012345012345/devices/dev_id",
		.response = "{ \
			     \"name\":\"DEVNAME\", \
			     \"model\":\"jsonrpcFullV1.0\", \
			     \"owner\":\"OWNER\", \
			     \"mtime\":\"123\", \
			     \"ctime\":\"456\", \
			     \"id\":\"dev_id\"}"
	},
};


#define nr_defaults sizeof(defaults)/sizeof(defaults[0])
static char* replace[nr_defaults];

static int _index_get_by_url(char *url)
{
	int i;
	for (i=0; i<nr_defaults; i++)
		if (strcmp(url, defaults[i].url) == 0)
			return i;

	return -1;
}


char *restapi_server_call(char *url)
{
	int i = _index_get_by_url(url);
	if (i == -1) {
		fprintf(stdout, "[ERR] restapi_server_call failed\n");
		return NULL;
	}

	if (replace[i]) {
		return replace[i];
	}
	else {
		return defaults[i].response;
	}
}

void restapi_server_response_set(char *url, char *response)
{
	int i = _index_get_by_url(url);
	if (i == -1)
		return;

	replace[i] = strdup(response);
	return;
}

void restapi_server_reset(void)
{
	int i;
	for (i=0;i <nr_defaults; i++) {
		if (replace[i]) {
			free(replace[i]);
			replace[i] = NULL;
		}
	}
}

char *restapi_server_expect_get(char *url)
{
	return restapi_server_call(url);
	/*
	int i = _index_get_by_url(url);
	if (i == -1)
		return;

	if (replace[i])
		return replace[i];
	else
		return defaults[i].response;
*/
}


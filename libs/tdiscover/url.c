#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "url.h"

#include "log/tube_log.h"

static const char *base_url = "https://api.thingplus.net/";
static struct url_list{
	enum url_index url_index;
	char *url;
} url_list[] = {
	{.url_index = URL_INDEX_GATEWAY_INFO, .url = "gateways/%s?fields=model&fields=autoCreateDiscoverable&fields=devices&fields=sensors"},
	{.url_index = URL_INDEX_GATEWAY_MODEL, .url = "gatewayModels/%d"},
	{.url_index = URL_INDEX_DEVICE_REGISTER, .url = "gateways/%s/devices"},
	{.url_index = URL_INDEX_SENSOR_DRIVERS, .url = "sensorDrivers/?filter[id]=%s"},
	{.url_index = URL_INDEX_SENSOR_REGISTER, .url ="gateways/%s/sensors"},
	{.url_index = URL_INDEX_DEVICE_INFO, .url ="gateways/%s/devices/%s"},
};

static struct url_list* _url_search(enum url_index r)
{
	int i;
	for (i=0; i<(int)(sizeof(url_list)/sizeof(url_list[0])); i++) {
		if (r == url_list[i].url_index) {
			return &url_list[i];
		}
	}
	return NULL;
}

char *url_get(enum url_index r, ...)
{
	int base_url_len = strlen(base_url);
	struct url_list *u = _url_search(r);
	if (u == NULL) {
		tube_log_error("[URL] _url_search failed. url_index is %d\n", r);
		return NULL;
	}

	char *url = calloc(1, base_url_len + strlen(u->url) * 4);
	if (url == NULL) {
		tube_log_error("[URL] calloc failed. size:%lu\n",
			base_url_len + strlen(u->url) * 2);
		return NULL;
	}

	va_list args;
	va_start(args, r);

	strncpy(url, base_url, base_url_len);
	vsprintf(&url[base_url_len], u->url, args);

	va_end(args);

	return url;
}

void url_put(char *url)
{
	if (!url)
		return ;

	free(url);
}

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "gateway.h"
#include "apc100.h"

bool _pcouner_is_on(void)
{
	return true;
}

static void _cb_pcounter_in(void *arg, struct apc100_cumulativeness *ac)
{
	char value[100] = {0,};
	sprintf(value, "%d", ac->in);
	gw_event_value_publish("countEvent-a820664d5df4", value);
}

int main(int argc, char **argv)
{
	char *gw_id;
	char *apikey;
	char *ca_cert;
	char *sensor_id;
	struct sensor s;

	gw_id =  "a820664d5df4";
	apikey = "7VYHKL-YKVp278d7zKOAtVOqruA=";
	ca_cert = "../../cert/ca-cert.pem";

	sensor_id = "countEvent-a820664d5df4";
	strcpy(s.id, sensor_id);
	s.ops.on = _pcouner_is_on;

	gw_init(gw_id, apikey, 1, &s);
	gw_connect(ca_cert);

	void *a = apc100_init("/dev/ttyUSB0");
	apc100_in_trigger(a, _cb_pcounter_in, NULL);

	while(1);
}

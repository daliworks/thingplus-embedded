#ifndef _GATEWAY_H_
#define _GATEWAY_H_

#define GW_NAMESPACE_LEN	512

struct sensor {
	char id[GW_NAMESPACE_LEN];
	struct ops{
		bool (*on)(void);
		char* (*get)(void);
		int (*set)(char *, char *);
	} ops;
};

int gw_event_value_publish(char *id, char *value);

int gw_connect(char *ca_cert);
void gw_disconnect(void);


int gw_init(char *gw_id, char *apikey, int nr_sensor, struct sensor *s);
void gw_cleanup(void);

#endif 

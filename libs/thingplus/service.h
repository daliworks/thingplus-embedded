#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "thingplus.h"

char *service_do(void *s, char *payload);
void *service_init(struct thingplus_callback *cb, void *cb_arg);
void service_cleanup(void *s);
#endif //#define _SERVICE_H_

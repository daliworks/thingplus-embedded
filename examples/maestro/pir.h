#ifndef _PIR_H_
#define _PIR_H_

void *pir_init(void* , void (*ops)(void*, char*));
int pir_get(void *);

#endif //#ifndef _PIR_H_

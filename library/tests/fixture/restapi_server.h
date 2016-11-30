#ifndef _RESTAPI_SERVER_H_
#define _RESTAPI_SERVER_H_

char* restapi_server_call(char *url);
char *restapi_server_expect_get(char *url);
void restapi_server_response_set(char *url, char *response);
void restapi_server_reset(void);

#endif //#ifndef _RESTAPI_SERVER_H_

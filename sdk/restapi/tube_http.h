#ifndef _TUBE_HTTP_H_
#define _TUBE_HTTP_H_

#define TUBE_HTTP_MAX_ID	128

enum tube_http_error
{
	TUBE_HTTP_ERROR_NO_ERROR = 0,
	TUBE_HTTP_ERROR_ALREADY_REGISTERED = -1,
	TUBE_HTTP_ERROR_NO_MEMORY = -2,
	TUBE_HTTP_ERROR_BILLING = -3,
	TUBE_HTTP_ERROR_INVALID_GATEWAY_ID = -4,
	TUBE_HTTP_ERROR_INVALID_TYPE = -5,
	TUBE_HTTP_ERROR_NOT_DISCOVERABLE = -6,
	TUBE_HTTP_ERROR_INVALID_ARGUMENT = -7,
	TUBE_HTTP_ERROR = -8,
	TUBE_HTTP_ERROR_DEVICE_MODEL_ID = -9,
	TUBE_HTTP_ERROR_CURL = -9,
};

enum tube_http_error tube_http_sensor_register(void* tube_http, char* name, int uid, char* type, char* device_id, char* sensor_id);
enum tube_http_error tube_http_device_register(void* tube_http, char* name, int uid, char* device_model_id, char* device_id);

char *tube_http_version(void);

void* tube_http_init(char *gateway_id, char *apikey);
void tube_http_cleanup(void* tube_http);

#endif  //#ifndef _TUBE_HTTP_H_

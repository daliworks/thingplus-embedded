#ifndef _TDISCOVER_H_
#define _TDISCOVER_H_

#define TDISCOVER_MAX_ID	128

enum tdiscover_error
{
	TDISCOVER_ERROR_NO_ERROR = 0,
	TDISCOVER_ERROR_ALREADY_REGISTERED = -1,
	TDISCOVER_ERROR_NO_MEMORY = -2,
	TDISCOVER_ERROR_BILLING = -3,
	TDISCOVER_ERROR_INVALID_GATEWAY_ID = -4,
	TDISCOVER_ERROR_INVALID_TYPE = -5,
	TDISCOVER_ERROR_NOT_DISCOVERABLE = -6,
	TDISCOVER_ERROR_INVALID_ARGUMENT = -7,
	TDISCOVER_ERROR = -8,
	TDISCOVER_ERROR_DEVICE_MODEL_ID = -9,
	TDISCOVER_ERROR_READ = -10,
	TDISCOVER_ERROR_POST = -11,
	TDISCOVER_ERROR_DUPLICATED = -12,
};

enum tdiscover_error tdiscover_sensor_register(void* instance, char* name, int uid, char* type, char* device_id, char* sensor_id);
enum tdiscover_error tdiscover_device_register(void* instance, char* name, int uid, char* device_model_id, char* device_id);

void* tdiscover_init(char *gateway_id, char *apikey);
void tdiscover_cleanup(void* instance);

#endif  //#ifndef _TDISCOVER_H_

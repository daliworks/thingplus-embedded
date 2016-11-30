#ifndef _REST_H_
#define _REST_H_

void rest_gatewayinfo_free(struct thingplus_gateway *gw_info);

int rest_gatewayinfo(void *instance, struct thingplus_gateway *gw_info);
int rest_sensorinfo(void *instance, char *id, struct thingplus_sensor *sensor_info);
int rest_deviceinfo(void *instance, char *id, struct thingplus_device *dev_info);
int rest_sensor_register(void* instance, char* name, int uid, char* type, char* device_id, char* sensor_id);
enum thingplus_error rest_device_register(void* instance, char* name, int uid, char* device_model_id, char* device_id);

void* rest_init(char *gw_id, char *apikey, char *rest_url);
void rest_cleanup(void* instance);

#endif //#ifndef _REST_H_

#ifndef _THINGPLUS_MOCK_EXPECT_H_
#define _THINGPLUS_MOCK_EXPECT_H_

void mock_expect_thingplus_init(char *gw_id, char *apikey);
void mock_expect_thingplus_cleanup(void);
void mock_expect_thingplus_connect_ssl(char *ca_file, char *mqtt_url, int keepalive);
void mock_expect_thingplus_connect_non_ssl(char *mqtt_url, int keepalive);
void mock_expect_thingplus_connected(char *gw_id);

#endif //#ifndef _THINGPLUS_MOCK_EXPECT_H_

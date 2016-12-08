#ifndef _MOSQUITTO_MOCK_H_
#define _MOSQUITTO_MOCK_H_

#define MOCK_MOSQUITTO_ERROR 0xdeedbeef

void mock_mosquitto_connected(int result);
void mock_mosquitto_disconnected(int result);
void mock_mosquitto_subscribed(char *topic, char *payload, int payload_len);
void mock_mosquitto_published(int mid);

#endif //#ifndef _MOSQUITTO_MOCK_H_

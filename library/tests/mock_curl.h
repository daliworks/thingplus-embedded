#ifndef _MOCK_CURL_H_
#define _MOCK_CURL_H_ //#ifndef _MOCK_CURL_H_

#define CURL_GLOBAL_ALL	0x3

void mock_curl_payload_set(char *contents);
void mock_curl_payload_append(char *payload);
void mock_curl_payload_clear(void);

#endif //#ifndef _MOCK_CURL_H_

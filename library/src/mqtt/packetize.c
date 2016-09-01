#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int packetize_payload(char **payload, int payload_size, char *format, ...)
{
#define DEFAULT_MESSAGE_LEN 512

	char message_temp[DEFAULT_MESSAGE_LEN] = {0, };
	va_list args;

	va_start(args, format);
	vsprintf(message_temp, format, args);
	va_end(args);

	int message_len = strlen(message_temp);
	int payload_available_len = 0;
	if (*payload)
		payload_available_len = payload_size - strlen(*payload);

	if (message_len + 1 > payload_available_len) {
		*payload = realloc(*payload, payload_size + DEFAULT_MESSAGE_LEN);
		if (*payload == NULL) {
			fprintf(stdout, "[ERR] realloc failed\n");
			return -1;
		}

		memset(&((*payload)[payload_size]), 0, DEFAULT_MESSAGE_LEN);
		payload_size += DEFAULT_MESSAGE_LEN;
	}

	strcpy(&((*payload)[strlen(*payload)]), message_temp);

	return payload_size;
}

void packetize_free(char *payload)
{
	if (!payload)
		return;

	free(payload);
}

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/tube_log.h"

int reassemble_payload(char **payload, int payload_size, char *format, ...)
{
#define DEFAULT_MESSAGE_LEN 512

	char message_temp[DEFAULT_MESSAGE_LEN] = {0, };
	va_list args;

	va_start(args, format);
	vsprintf(message_temp, format, args);
	va_end(args);

	//tube_log_debug("message_temp:%s\n", message_temp);
	int message_len = strlen(message_temp);
	int payload_available_len = 0;
	if (*payload)
		payload_available_len = payload_size - strlen(*payload);

	if (message_len + 1 > payload_available_len) {
		*payload = realloc(*payload, payload_size + DEFAULT_MESSAGE_LEN);
		if (*payload == NULL) {
			tube_log_error("[MQTT] realloc failed\n");
			return -1;
		}
		memset(&((*payload)[payload_size]), 0, DEFAULT_MESSAGE_LEN);

		payload_size += DEFAULT_MESSAGE_LEN;
	}

	strcpy(&((*payload)[strlen(*payload)]), message_temp);

	return payload_size;
}

int reassemble_free(char *payload)
{
	if (!payload)
		return;

	free(payload);
}

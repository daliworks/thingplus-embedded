#ifndef _PACKETIZE_H_
#define _PACKETIZE_H_

int packetize_payload(char **payload, int payload_size, char *format, ...);
void packetize_free(char *payload);

#endif //#ifndef _PACKETIZE_H_

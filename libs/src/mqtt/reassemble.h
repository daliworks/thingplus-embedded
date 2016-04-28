#ifndef _REASSEMBLE_H_
#define _REASSEMBLE_H_

int reassemble_payload(char **payload, int payload_size, char *format, ...);
int reassemble_free(char *payload);

#endif //#ifndef _REASSEMBLE_H_

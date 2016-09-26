#ifndef _HTX23C_H_
#define _HTX23C_H_

#define HTX23C_ERROR	 0xffff

int htx23c_temperature(void* htx23c);
int htx23c_humidity(void* htx23c);

void htx23c_cleanup(void* htx23c);
void* htx23c_init(char *tty, int slave_id);

#endif //#ifndef _HTX23C_H_

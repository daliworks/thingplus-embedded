#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <modbus/modbus.h>

#include "htx23c.h"

#if 0
#define HTX23C_CURRENT_HUMIDITY_ADDRESS		1
#define HTX23C_CURRENT_TEMPERATURE_ADDRESS	5
#else
#define HTX23C_CURRENT_HUMIDITY_ADDRESS		21
#define HTX23C_CURRENT_TEMPERATURE_ADDRESS	22
#endif
#define HTX23C_POLL_INTERVAL_US			100000

struct htx23c {
	modbus_t *ctx;
};

int htx23c_temperature(void* _htx23c)
{
	if (_htx23c == NULL) {
		return HTX23C_ERROR;
	}

	usleep(HTX23C_POLL_INTERVAL_US);

	struct htx23c *h = (struct htx23c*)_htx23c;
	uint16_t temperature;
	if (modbus_read_registers(h->ctx, HTX23C_CURRENT_TEMPERATURE_ADDRESS, 1, &temperature) < 0) {
		fprintf(stdout, "modbus_read_registers failed. reg:%x\n", HTX23C_CURRENT_TEMPERATURE_ADDRESS);
		temperature = HTX23C_ERROR;
	}

	return temperature;
}

int htx23c_humidity(void* _htx23c)
{
	if (_htx23c == NULL) {
		return HTX23C_ERROR;
	}

	usleep(HTX23C_POLL_INTERVAL_US);

	struct htx23c *h = (struct htx23c*)_htx23c;
	uint16_t humidity;
	if (modbus_read_registers(h->ctx, HTX23C_CURRENT_HUMIDITY_ADDRESS, 1, &humidity) < 0) {
		fprintf(stdout, "modbus_read_registers failed. reg:%x\n", HTX23C_CURRENT_HUMIDITY_ADDRESS);
		humidity = HTX23C_ERROR;
	}

	return humidity;
}

void* htx23c_init(char *tty, int slave_id)
{
	if (!tty) {
		return NULL;
	}

	if (access(tty, F_OK) < 0) {
		fprintf(stdout, "%s is not exists", tty);
		return NULL;
	}

	struct htx23c* h = (struct htx23c*)calloc(1, sizeof(struct htx23c));
	if (!h) {
		fprintf(stdout, "calloc failed. sizeof:%ld\n", sizeof(struct htx23c));
		return NULL;
	}

	h->ctx = modbus_new_rtu(tty, 9600, 'N', 8, 1);
	if (!h->ctx) {
		fprintf(stdout, "modbus_new_rtu failed\n");
		return NULL;
	}

	modbus_set_slave(h->ctx, slave_id);
	modbus_connect(h->ctx);

	return (void*)h;
}

void htx23c_cleanup(void* htx23c)
{
	if (htx23c == NULL)
		return;

	struct htx23c* h = (struct htx23c*)htx23c;

	modbus_close(h->ctx);
	modbus_free(h->ctx);
	free(h);
}

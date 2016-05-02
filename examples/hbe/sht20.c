#include <stdio.h>
#include <stdint.h>
#include <wiringPiI2C.h>

int fd;

int sht20_init(void)
{
	fd = wiringPiI2CSetup(0x40);
}

int sht20_temperature_get(void)
{
	wiringPiI2CWrite(fd, 0xf3);

	usleep(86 * 1000);

	int msb = wiringPiI2CRead(fd);
	int lsb = wiringPiI2CRead(fd);
	printf("msb:%x lsb:%x\n", msb, lsb);

	int value = msb << 8 | lsb;
	value &= ~(int)0x03;
	printf("value:%x\n", value);

	int temperature = -46.85 + (((int)(175.72* value)) >> 16 );

	printf("t:%d\n", temperature);
	return temperature;
}

int sht20_humidity_get(void)
{
	wiringPiI2CWrite(fd, 0xf5);

	usleep(29 * 1000);

	int msb = wiringPiI2CRead(fd);
	int lsb = wiringPiI2CRead(fd);

	printf("%x %x\n", msb, lsb);

	int value = msb << 8 | lsb;
	value &= ~(int)0x03;
	printf("value:%x\n", value);

	int humidity = -6 + ((125 * value) >> 16);

	printf("h:%d\n", humidity);

	return humidity;
}

#if 0
int main(void)
{
	sht20_init();
	sht20_temperature_get();
	sht20_humidity_get();
}
#endif

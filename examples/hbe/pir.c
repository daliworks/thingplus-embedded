#include <stdio.h>
#include <wiringPi.h>
#include <time.h>

void *cb_arg = NULL;
void (*cb)(void*, char*) = NULL;

#define GPIO 7
int prev_value = 0;
int value = 0;

PI_THREAD (monitoring)
{
	while (1) {
		value = digitalRead(GPIO);
		printf("1.%d\n", value);
		if (prev_value != value) {
			sleep(1);
			value = digitalRead(GPIO);
			printf("2.%d\n", value);
			if (prev_value != value) {
				prev_value = value;
				printf("pir changed\n");
				cb(cb_arg, (char*)value);
			}
		}
		else {
			usleep (500 * 1000);
		}
	}
}

void *pir_init(void* arg, void (*ops)(void*, char*))
{
	cb_arg = arg;
	cb = ops;
	wiringPiSetup();
	pinMode(GPIO, INPUT);
	piThreadCreate(monitoring);
	prev_value = digitalRead(GPIO);
}

int pir_get(void* p)
{
	return value;
}

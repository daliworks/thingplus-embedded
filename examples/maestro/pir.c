#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

void *cb_arg = NULL;
void (*cb)(void*, char*) = NULL;
char last_value = '2';

#define GPIO 30

static unsigned long long _now_ms(void)
{
        struct timeval tv;
        unsigned long long now;

        gettimeofday(&tv, NULL);
        now = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

        return now;
}

static void _monitoring(void *arg)
{
	while (1) {
		FILE *gpio = fopen("/sys/class/gpio/gpio30/value", "r");
		char value = NULL;

		if (gpio != NULL) {

			fread(&value, 1, 1, gpio);

			if (value != last_value) {
				last_value = value;
				value = value - '0';
				char reversed_value = value ? 0 : 1;
				cb(cb_arg, (char*)reversed_value);
			}
			fclose(gpio);
		}

		usleep(1000 * 100);
	}

	return;
}

void *pir_init(void* arg, void (*ops)(void*, char*))
{
	pthread_t tid;
	cb_arg = arg;
	cb = ops;

	pthread_create(&tid, NULL, _monitoring, NULL);


	/*
	wiringPiSetup();
	pinMode(GPIO, INPUT);
	piThreadCreate(monitoring);
	*/
}

int pir_get(void* p)
{
	return 0;
}

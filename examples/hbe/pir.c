#include <stdio.h>
#include <wiringPi.h>
#include <time.h>
#include <sys/time.h>

void *cb_arg = NULL;
void (*cb)(void*, char*) = NULL;

#define GPIO 7

static unsigned long long _now_ms(void)
{
        struct timeval tv;
        unsigned long long now;

        gettimeofday(&tv, NULL);
        now = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

        return now;
}

PI_THREAD (monitoring)
{
	int edge_count = 0;
	int detection;
	int value = 0;
	int prev = digitalRead(GPIO);

	unsigned long long now;
	unsigned long long trigger_time = _now_ms();

	while (1) {
		now = _now_ms();

		value = digitalRead(GPIO);
		if (value != prev)
			edge_count++;

		prev = value;

		if (now >= trigger_time + 500) {
			//printf("edge_count : %d\n", edge_count);
			if (edge_count >= 8 && detection == 0) {
				//printf("detect\n");
				detection = 1;
				cb(cb_arg, (char*)detection);
				sleep(5);
			}
			else if (detection == 1) {
				//printf("undetect\n");
				detection = 0;
				cb(cb_arg, (char*)detection);
			}

			edge_count = 0;
			trigger_time = _now_ms();
		}

		usleep(1 * 1000);
	}
}

void *pir_init(void* arg, void (*ops)(void*, char*))
{
	cb_arg = arg;
	cb = ops;
	wiringPiSetup();
	pinMode(GPIO, INPUT);
	piThreadCreate(monitoring);
}

int pir_get(void* p)
{
	return digitalRead(GPIO);
}

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "apc100.h"

#define APC100_RESPONSE_LENGTH	43

struct apc100{
	int fd;
	struct apc100_cumulativeness cumulativenss;

	pthread_t tid;
	pthread_mutex_t cumulativeness_lock;

	void *cb_in_arg;
	void (*cb_in)(void*, struct apc100_cumulativeness *);

};

static void _noncanonical(struct termios *t)
{
	t->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
}

static void _baudrate_115200(struct termios *t)
{
	cfsetispeed(t, B9600);
	cfsetospeed(t, B9600);
}

static void _8N1(struct termios *t)
{
	t->c_cflag &= ~PARENB;
	t->c_cflag &= ~CSTOPB;
	t->c_cflag &= ~CSIZE;
	t->c_cflag |= CS8;
}

static void _flowcontrol_off(struct termios *t)
{
	t->c_cflag &= ~CRTSCTS;
	t->c_cflag |= CREAD | CLOCAL;
	t->c_cflag &= ~(IXON | IXOFF | IXANY);

	t->c_cc[VMIN] = APC100_RESPONSE_LENGTH;
	t->c_cc[VTIME] = 0;
}

static void _response_parse(char *response, struct apc100_cumulativeness *c)
{
	char in_str[6] = {0,};
	char out_str[6] = {0,};
	char count_str[6] = {0,};

	memcpy(in_str, &response[9], 5);
	memcpy(out_str, &response[15], 5);
	memcpy(count_str, &response[21], 5);

	c->in = atoi(in_str);
	c->out = atoi(out_str);
	c->count = atoi(count_str);
}

static void* _polling(void *arg)
{
	struct apc100 *a = arg;
	struct apc100_cumulativeness c;

	while (1) {
		if (apc100_cumulativeness(a, &c) < 0)
			goto yield;

		if (memcmp(&a->cumulativenss, &c, sizeof(struct apc100_cumulativeness)) == 0)
			goto yield;

		if (a->cumulativenss.in != c.in) {
			if (a->cb_in)
				a->cb_in(a->cb_in_arg, &c);
		}

		memcpy(&a->cumulativenss, &c, sizeof(struct apc100_cumulativeness));

yield:
		sched_yield();
		pthread_testcancel();
	}

	return NULL;
}

static void _polling_cancel(struct apc100 *a)
{
	pthread_cancel(a->tid);
	pthread_join(a->tid, NULL);
}

static void _polling_start(struct apc100 *a)
{
	pthread_create(&a->tid, NULL, _polling, a);
}

int apc100_cumulativeness(void *_a, struct apc100_cumulativeness *c)
{
#define APC100_PREPARATION_WAIT() 	usleep(1000)

	if (_a == NULL) {
		return -1;
	}

	struct apc100 *a = _a;

	pthread_mutex_lock(&a->cumulativeness_lock);

	APC100_PREPARATION_WAIT();

	char *cmd = "[0000BTR]";
	int written = write(a->fd, cmd, strlen(cmd));
	if (written != strlen(cmd)) {
		printf("[ERR] written failed. %d\n", written);
		goto err_write;
	}

	char response[APC100_RESPONSE_LENGTH + 1] = {0,};
	int nr_read = read(a->fd, response, sizeof(response));
	if (nr_read != APC100_RESPONSE_LENGTH) {
		printf("[ERR] read failed. %d\n", nr_read);
		goto err_read;
	}
	pthread_mutex_unlock(&a->cumulativeness_lock);
	printf("%s\n", response);

	_response_parse(response, c);

	return 0;

err_write:
err_read:
	pthread_mutex_unlock(&a->cumulativeness_lock);
	return -1;
}

int apc100_in_trigger(void *_a, void (*cb)(void *, struct apc100_cumulativeness*), void *arg)
{
	if (_a == NULL) {
		return -1;
	}
	struct apc100* a = _a;
	a->cb_in = cb;
	a->cb_in_arg = arg;

	return 0;
}

void *apc100_init(char *tty)
{
	if (tty == NULL) {
		printf("[ERR] Invalid tty\n");
		return NULL;
	}

	struct apc100 *a = calloc(1, sizeof(struct apc100));
	if (a == NULL) {
		printf("[ERR] calloc failed. size:%ld\n", sizeof(struct apc100));
		return NULL;
	}

	a->fd = open(tty, O_RDWR|O_NOCTTY);
	if (a->fd == -1) {
		printf("[ERR] tty open failed\n");
		goto err_open;
	}

	struct termios termios;
	tcgetattr(a->fd, &termios);

	_baudrate_115200(&termios);
	_8N1(&termios);
	_flowcontrol_off(&termios);
	_noncanonical(&termios);

	if (tcsetattr(a->fd, TCSANOW, &termios) < 0) {
		printf("[ERR] tcsetattr failed\n");
		goto err_tcsetattr;
	}

	pthread_mutex_init(&a->cumulativeness_lock, NULL);

	_polling_start(a);

	return a;

err_tcsetattr:
	close(a->fd);

err_open:
	free(a);
	return NULL;
}

void apc100_cleanup(void *_a)
{
	if (!_a)
		return;

	struct apc100 *a = _a;

	_polling_cancel(a);

	close(a->fd);
	free(a);
}

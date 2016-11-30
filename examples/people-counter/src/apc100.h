#ifndef _APC100_H_
#define _APC100_H_

struct apc100_cumulativeness {
	int in;
	int out;
	int count;
};

int apc100_cumulativeness(void *, struct apc100_cumulativeness *);
int apc100_in_trigger(void *_a, void (*cb)(void *, struct apc100_cumulativeness*), void *arg);

void *apc100_init(char *tty);
void apc100_cleanup(void *);

#endif //#ifndef _APC100_H_

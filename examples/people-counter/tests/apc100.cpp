#include <CppUTest/TestHarness.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>

extern "C" {
#include <pthread.h>
#include <stdio.h>
#include "../src/apc100.h"
}

TEST_GROUP(APC100_INIT_CLEANUP) {
};

TEST(APC100_INIT_CLEANUP, valid_tty)
{
	void *a = apc100_init("/dev/ttyUSB0");
	CHECK(a);

	apc100_cleanup(a);
}

TEST(APC100_INIT_CLEANUP, invalid_tty)
{
	void *a = apc100_init(NULL);
	CHECK_FALSE(a);

	a = apc100_init("/dev/ttyDummy");
	CHECK_FALSE(a);
}

TEST(APC100_INIT_CLEANUP, cleanup_with_null_no_segmentfault)
{
	apc100_cleanup(NULL);
}

static pthread_mutex_t _asyncronous_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void _asyncronous_wait(void)
{
	int ret = pthread_mutex_lock(&_asyncronous_mutex);
}
static inline void _asyncronous_done(void)
{
	pthread_mutex_unlock(&_asyncronous_mutex);
}
#define _asyncronous_setup _asyncronous_wait 
#define _asyncronous_teardown _asyncronous_done

TEST_GROUP(apc100_cumulativeness)
{
	void *a;
	void setup()
	{
		_asyncronous_setup();
		a = apc100_init("/dev/ttyUSB0");
	}

	void teardown()
	{
		apc100_cleanup(a);
		_asyncronous_teardown();
	}
};

TEST(apc100_cumulativeness, call)
{
	struct apc100_cumulativeness c;
	int ret = apc100_cumulativeness(a, &c);
	CHECK_EQUAL(0, ret);
}

static void _cb_in(void *arg, struct apc100_cumulativeness* c)
{
	_asyncronous_done();
}

TEST(apc100_cumulativeness, in_trigger)
{
	int ret = apc100_in_trigger(a, _cb_in, NULL);
	CHECK_EQUAL(0, ret);
	_asyncronous_wait();
}

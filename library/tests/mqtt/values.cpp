#include "CppUTest/TestHarness.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"

extern "C" {
#include "mqtt/values.h"
#include "thingplus.h"
}

TEST_GROUP(values) {
};

TEST(values, single_sensor_values_init)
{
	int nr_value = 1;
	struct thingplus_value v[10];

	int i;
	for (i=0; i<nr_value; i++) {
		v[i].id ="a";
		v[i].value = "a";
		v[i].time_ms = i;
	}

	void *s = values_init(nr_value, v);
	CHECK(s);

	struct values_sensor *values_s;
	values_for_each_sensor(values_s, s) {
		STRCMP_EQUAL("a", values_s->id);

		CHECK_EQUAL(1, values_s->nr_value);
		CHECK_EQUAL(10, values_s->nr_allocated);

		int i;
		for (i=0; i<values_s->nr_value; i++) {
			STRCMP_EQUAL("a", values_s->value[i].id);
			STRCMP_EQUAL("a", values_s->value[i].value);
			CHECK_EQUAL(i, values_s->value[i].time_ms);
		}
	}
}

TEST(values, single_sensor_two_value_values_init)
{
	int nr_value = 2;
	struct thingplus_value v[10];

	int i;
	for (i=0; i<nr_value; i++) {
		v[i].id ="a";
		v[i].value = "a";
		v[i].time_ms = i;
	}

	void *s = values_init(nr_value, v);
	CHECK(s);

	struct values_sensor *values_s;
	values_for_each_sensor(values_s, s) {
		STRCMP_EQUAL("a", values_s->id);

		CHECK_EQUAL(nr_value, values_s->nr_value);
		CHECK_EQUAL(10, values_s->nr_allocated);

		int i;
		for (i=0; i<values_s->nr_value; i++) {
			STRCMP_EQUAL("a", values_s->value[i].id);
			STRCMP_EQUAL("a", values_s->value[i].value);
			CHECK_EQUAL(i, values_s->value[i].time_ms);
		}
	}
}

TEST(values, two_sensor_single_value_values_init)
{
	int nr_value = 2;
	struct thingplus_value v[10];

	v[0].id ="a";
	v[0].value = "a";
	v[0].time_ms = 1000;

	v[1].id ="b";
	v[1].value = "b";
	v[1].time_ms = 1000;

	void *s = values_init(nr_value, v);
	CHECK(s);

	struct values_sensor *values_s;
	int i = 0;
	values_for_each_sensor(values_s, s) {
		if (i++ == 1) {
			STRCMP_EQUAL("b", values_s->id);
			STRCMP_EQUAL("b", values_s->value[0].id);
			STRCMP_EQUAL("b", values_s->value[0].value);
			CHECK_EQUAL(1000, values_s->value[0].time_ms);
		}
		else  {
			STRCMP_EQUAL("a", values_s->id);
			STRCMP_EQUAL("a", values_s->value[0].id);
			STRCMP_EQUAL("a", values_s->value[0].value);
			CHECK_EQUAL(1000, values_s->value[0].time_ms);
		}

		CHECK_EQUAL(1, values_s->nr_value);
		CHECK_EQUAL(10, values_s->nr_allocated);
	}
}

TEST(values, cleanupNoMemoryLeadk)
{
	int nr_value = 2;
	struct thingplus_value v[10];

	v[0].id ="a";
	v[0].value = "a";
	v[0].time_ms = 1000;

	v[1].id ="b";
	v[1].value = "b";
	v[1].time_ms = 1000;
	void *s = values_init(nr_value, v);

	CHECK(s);
	values_cleanup(s);
}

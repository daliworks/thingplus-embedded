#include "CppUTest/CommandLineTestRunner.h"

int main(int ac, char **av)
{
	return CommandLineTestRunner::RunAllTests(ac, av);
}

#if 0
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"



TEST_GROUP_C_WRAPPER(INIT_CLEANUP) {
	TEST_GROUP_C_SETUP_WRAPPER(INIT_CLEANUP);
	TEST_GROUP_C_TEARDOWN_WRAPPER(INIT_CLEANUP);
	/*
	void setup()
	{
	}

	void teardown()
	{
		MemoryLeakWarningPlugin::turnOnNewDeleteOverloads();
	}
	*/
};

TEST_C_WRAPPER(INIT_CLEANUP, SUCCESS);
TEST_C_WRAPPER(INIT_CLEANUP, CA_FILE_NULL_INIT_SUCCESS);
TEST_C_WRAPPER(INIT_CLEANUP, ERR_CA_FILE);
TEST_C_WRAPPER(INIT_CLEANUP, ERR_GW_ID);
TEST_C_WRAPPER(INIT_CLEANUP, ERR_APIKEY);
TEST_C_WRAPPER(INIT_CLEANUP, ERR_MQTT_URL);
TEST_C_WRAPPER(INIT_CLEANUP, ERR_RESTAPI_URL);

int main(int ac, char **av)
{
	MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
	RUN_ALL_TESTS(ac, av);
	MemoryLeakWarningPlugin::turnOnNewDeleteOverloads();
}
#endif

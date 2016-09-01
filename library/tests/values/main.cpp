#include "CppUTest/CommandLineTestRunner.h"

#ifdef __cplusplus
#pragma GCC diagnostic ignored "-fpermissive"
#endif

int main(int ac, char **av)
{
	return CommandLineTestRunner::RunAllTests(ac, av);
}

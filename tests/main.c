#include "test.h"

#ifdef _WIN32
int __cdecl main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	int res;

	clar_test_init(argc, argv);
	res = clar_test_run();
	clar_test_shutdown();

	return res;
}

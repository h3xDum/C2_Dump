#include "env.hpp"


int hardware_check() {

	/* check cpu cores */
	unsigned int threads_count = std::thread::hardware_concurrency();
	if (threads_count < 4) {
		return 1;
	}

	/* check memory size */
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	if ((float)status.ullTotalPhys / (1024 * 1024 * 1024) < 3.7) {
		return 1;
	}

	return 0;
}

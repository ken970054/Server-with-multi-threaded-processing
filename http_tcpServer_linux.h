#ifndef INCLUDED_HTTP_TCPSERVER_LINUX
#define INCLUDED_HTTP_TCPSERVER_LINUX

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>

#include "parse_clientRequest.h"

class TcpServer {
public:
    TcpServer(std::string ip_address, int port, std::string document_root);
    ~TcpServer();
    void startListen(Parser *P);
private:
    std::string _document_root;
     
    std::string m_ip_address;
    int m_port;
    int m_socket;
    int m_new_socket;
    long m_incomingMessage;
    struct sockaddr_in m_socketAddress;
    unsigned int m_socketAddress_len;
    std::string m_serverMessage;

    int startServer();
    void closeServer();
    void acceptConnection(int &new_socket);
    void parseAndProcessRequest();
    std::string readFile(std::string path);
    void sendResponse();
};


#endif

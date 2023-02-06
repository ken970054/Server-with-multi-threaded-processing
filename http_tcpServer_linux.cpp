#include "http_tcpServer_linux.h"
#include "parse_clientRequest.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <regex>

void log(const std::string &message) {
    std::cout << message << std::endl;
}

void exitWithError(const std::string &errorMessage) {
    log("ERROR: " + errorMessage);
    exit(1);
}

TcpServer::TcpServer(std::string ip_address, int port, std::string document_root) : m_ip_address(ip_address), m_port(port), _document_root(document_root),
                                                                                    m_socket(),
                                                                                    m_new_socket(),
                                                                                    m_incomingMessage(),
                                                                                    m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)),
                                                                                    m_serverMessage()
{
    m_socketAddress.sin_family = AF_INET;
    m_socketAddress.sin_port = htons(m_port);
    m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

    if (startServer() != 0) {
        std::ostringstream ss;
        ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
        log(ss.str());
    }
}

TcpServer::~TcpServer() {
    closeServer();
}

int TcpServer::startServer() {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        exitWithError("Cannot create socket");
        return 1;
    }

    if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0) {
        exitWithError("Cannot connect socket to address");
        return 1;
    }

    return 0;
}

void TcpServer::closeServer() {
    close(m_socket);
    close(m_new_socket);
    exit(0);
}

void TcpServer::startListen(Parser *P) {
    if (listen(m_socket, 100) < 0) {
        exitWithError("Socket listen failed");
    }
    std::ostringstream ss;
    ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
    log(ss.str());

    while (true) {
        log("====== Waiting for a new connection ======\n\n\n");
        
        acceptConnection(m_new_socket);

        // Generate request time
        char timestr[200];
        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        strftime(timestr, 200, "%a, %d %b %Y %H:%M:%S %Z", &tm);
        std::string requestTime(timestr);

        // Store client information into clientInfo
        clientIdentity cid;
        cid.ip = m_ip_address;
        cid.portno = m_port;
        cid.requestTime = requestTime;

        P->parseRequest(cid, m_new_socket, _document_root);

        //parseAndProcessRequest();

        log("------ Received Request from client ------\n\n");

        //sendResponse();

        //close(m_new_socket);
    }
}

void TcpServer::acceptConnection(int &new_socket) {
    new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
    if (new_socket < 0) {
        std::ostringstream ss;
        ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
        exitWithError(ss.str());
    }
}


std::string TcpServer::readFile(std::string path) {
    std::size_t readSize = 4096;
    std::ifstream rf;
    rf.open(path);
    
    std::string out;
    std::string buf(readSize, '\0');
    while (rf.read(& buf[0], readSize)) {
        out.append(buf, 0, rf.gcount());
    }
    out.append(buf, 0, rf.gcount());
    rf.close();
    return out;
}

void TcpServer::sendResponse() {
    long bytesSent;
    bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
    if (bytesSent == m_serverMessage.size()) {
        log("------ Server Response sent to client ------\n\n");
    } else {
        log("Error sending response to client");
    }
}

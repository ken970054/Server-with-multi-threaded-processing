#include "http_tcpServer_linux.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <vector>
#include <regex>

namespace {
    const int BUFFER_SIZE = 4096;
    void log(const std::string &message) {
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage) {
        log("ERROR: " + errorMessage);
        exit(1);
    }
}


namespace http {
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

    void TcpServer::startListen() {
        if (listen(m_socket, 20) < 0) {
            exitWithError("Socket listen failed");
        }
        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        while (true) {
            log("====== Waiting for a new connection ======\n\n\n");

            acceptConnection(m_new_socket);

            parseAndProcessRequest();

            log("------ Received Request from client ------\n\n");

            sendResponse();

            close(m_new_socket);
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

    void TcpServer::parseAndProcessRequest() {

        /* process received client request */
        char* recv_client_msg = new char[BUFFER_SIZE];
        int rc = read(m_new_socket, recv_client_msg, BUFFER_SIZE);
        if (rc < 0) {
            exitWithError("Failed to read bytes from client socket connection");
        }
        /* Generate Date for header*/
        char timestr[200];
        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        strftime(timestr, 200, "%a, %d %b %Y %H:%M:%S %Z", &tm);

        //char t1[10], t2[200], t3[10];  /* GET, resource, HTTP/1.0 */
        //sscanf(recv_client_msg, "%s %s %s", t1, t2, t3);
        std::string recv_client_msg_s = recv_client_msg;
        std::istringstream iss(recv_client_msg_s);
        std::string clientRequest1;
        std::string clientRequest2;
        std::string clientRequest3;
        iss >> clientRequest1 >> clientRequest2 >> clientRequest3; 
        
        std::cout << clientRequest1 << " " << clientRequest2 << " " << clientRequest3 << std::endl; 
        
        std::ostringstream responseMessage;

        // GET request: reponse header + body, HEAD request: reponse header only
        if (clientRequest1.compare("GET") == 0 && (clientRequest3.compare("HTTP/1.0") == 0 || clientRequest3.compare("HTTP/1.1") == 0)) {
            if (clientRequest2 == "/") clientRequest2 = "/index.html";
            std::string filePath = _document_root + clientRequest2;
            // some files name have space included, and URL will replace to "%20". Therefore, we need to replace back to space for file process
            filePath = std::regex_replace(filePath, std::regex("%20"), " ");
            std::cout << filePath << std::endl; 
            struct stat sb;
            std::string readContent;

            std::string fileType;
            std::string contentType;
            // check if requested file exist
            if (stat(filePath.c_str(), &sb) == 0) {

                /* check content type and build HTTP header */

                // find file type based on clientRequest2 
                std::string cr2_copy = clientRequest2;
                std::string delimiter = ".";
                std::size_t pos = 0;
                while ((pos = cr2_copy.find(delimiter)) != std::string::npos) {
                    cr2_copy.erase(0, pos + delimiter.length());
                }
                fileType = cr2_copy;
                //std::cout << fileType << std::endl;
                
                if (fileType == "html") {
                    contentType = "text/html";
                } else if (fileType == "txt") {
                    contentType = "text/txt";
                } else if (fileType == "js") {
                    contentType = "text/js";
                } else if (fileType == "css") {
                    contentType = "text/css";
                } else if (fileType == "jpg") {
                    contentType = "image/jpg";
                } else if (fileType == "gif") {
                    contentType = "image/gif";
                } else if (fileType == "png") {
                    contentType = "image/png";
                } else if (fileType == "svg") {
                    contentType = "image/svg";
                } else { // 400 bad request
                    responseMessage << bad_request_response << timestr << "\r\n\r\n" << bad_request_body << "\n";
                    m_serverMessage = responseMessage.str();
                    write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
                    return;
                }

                readContent = readFile(filePath);
                std::cout << readContent << std::endl;
                responseMessage << clientRequest3 << " 200 Ok\r\nContent-type: " << contentType << "\r\nContent-length: " << readContent.size() << "\r\nDate: " << timestr << "\r\n\r\n\n" << readContent << "\n";
                m_serverMessage = responseMessage.str();
                //std::cout << m_serverMessage << std::endl;
                //write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
            // 404 not found
            } else {
                responseMessage << not_found_response << timestr << "\r\n\r\n" << not_found_body << "\n";
                m_serverMessage = responseMessage.str();
                //write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
            }
        // 400 bad request 
        } else {
            responseMessage << bad_request_response << timestr << "\r\n\r\n" << bad_request_body << "\n";
            m_serverMessage = responseMessage.str();
            //write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
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
}

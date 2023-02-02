#ifndef INCLUDED_HTTP_TCPSERVER_LINUX
#define INCLUDED_HTTP_TCPSERVER_LINUX

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>

namespace http {
    class TcpServer {
    public:
        TcpServer(std::string ip_address, int port, std::string document_root);
        ~TcpServer();
        void startListen();
    private:
        /* strings for 400, 403, 404 responses */
        std::string not_found_response =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 122\r\n"
            "Date: ";
        std::string not_found_body =
            "<html><head><title>404: Not Found</title></head>\r\n"
            "<body><h1>Not Found</h1><p>Requested URL not found</p></body>\r\n"
            "</html>\r\n";
        std::string forbidden_response = 
            "HTTP/1.0 403 Forbidden\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 151\r\n"
            "Date: ";
        std::string forbidden_body =
            "<html><head><title>403: Forbidden</title></head>\r\n"
            "<body><h1>Forbidden</h1><p>You do not not have permission to view this resource</p>\r\n"
            "</body></html>\r\n";
        std::string bad_request_response =
            "HTTP/1.0 400 Bad Request\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 119\r\n"
            "Date: ";
        std::string bad_request_body =
            "<html><head><title>400: Bad Request</title></head>\r\n"
            "<body><h1>Bad request</h1><p>Bad HTTP request</p></body>\r\n"
            "</html>\r\n";
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

} // namespace http

#endif

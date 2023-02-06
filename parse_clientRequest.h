#ifndef PARSE_CLIENTREQUEST_H
#define PARSE_CLIENTREQUEST_H

#include <stdio.h>
#include <sys/socket.h>
#include <string>
#include <deque>


struct clientIdentity {
	int acceptId;
    std::string ip;
	int portno;
    std::string requestTime;
};

struct clientInfo {
    std::string r_ip;
    int r_portno;
    std::string r_requestTime;

    std::string r_contentType;
    std::string r_httpVersion;
    std::string r_filepath;
    std::string r_filename;
    std::string serveTime;
    int r_filesize;
    int r_acceptid;
    std::string status_code;
};

class Parser {
public:
    std::deque<clientInfo> clientList; 
    std::deque<clientInfo> requestList; 
    //Parser();
    //~Parser();
    static void *popRequest_helper(void *c);
    static void *serveRequest_helper(void *c);
    void parseRequest(clientIdentity c_id, int &m_new_socket, std::string document_root);
private:
    void checkRequest(clientInfo c_info, std::string recv_client_msg);
    void readyQueue(clientInfo c_info);

    void popRequest();
    void serveRequest();
    void sendData(clientInfo c_info);
    std::string readFile(std::string path);
    //bool fileExists(std::string filename);
};

#endif

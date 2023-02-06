#include "http_tcpServer_linux.h"
#include "parse_clientRequest.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

extern int BUFFER_SIZE;
extern int s_time;
extern pthread_mutex_t rqueue_lock;
extern pthread_cond_t rqueue_cond;
extern pthread_mutex_t send_lock;
extern pthread_cond_t send_cond;

void log(const std::string &message);
void exitWithError(const std::string &errorMessage);

void Parser::parseRequest(clientIdentity c_id, int &m_new_socket, std::string document_root) {
    clientInfo c_info;
    // copy client ip, port, document root and request time to c_info
    c_info.r_ip = c_id.ip;
    c_info.r_portno = c_id.portno;
    c_info.r_acceptid = m_new_socket;
    c_info.r_filepath = document_root;
    c_info.r_requestTime = c_id.requestTime;


    /* process received client request */
    char* recv_client_msg = new char[BUFFER_SIZE];
    int rc = read(m_new_socket, recv_client_msg, BUFFER_SIZE);
    if (rc < 0) {
        exitWithError("Failed to read bytes from client socket connection");
    }


    std::string recv_client_msg_s = recv_client_msg;
    checkRequest(c_info, recv_client_msg_s);

        
}

void Parser::checkRequest(clientInfo c_info, std::string recv_client_msg) {
    // parse first line of the message from client which is "GET /filemane HTTP/1.x"
    std::istringstream iss(recv_client_msg);
    std::string clientRequest1;
    std::string clientRequest2;
    std::string clientRequest3;
    iss >> clientRequest1 >> clientRequest2 >> clientRequest3; 
    
    std::cout << clientRequest1 << " " << clientRequest2 << " " << clientRequest3 << std::endl; 
    
    //std::ostringstream responseMessage;

    // check if the request is valid
    if (clientRequest1.compare("GET") == 0 && (clientRequest3.compare("HTTP/1.0") == 0 || clientRequest3.compare("HTTP/1.1") == 0)) {
        if (clientRequest2 == "/") clientRequest2 = "/index.html";
        std::string filePath = c_info.r_filepath + clientRequest2;
        // some files name have space included, and URL will replace to "%20". Therefore, we need to replace back to space for file process
        filePath = std::regex_replace(filePath, std::regex("%20"), " ");
        //std::cout << filePath << std::endl; 
        struct stat sb;
        std::string readContent;

        std::string fileType;
        std::string contentType;
        // check if requested file exist
        if (stat(filePath.c_str(), &sb) == 0) {
            c_info.r_filename = clientRequest2;
            c_info.r_httpVersion = clientRequest3;

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
                c_info.status_code = "400";
                readyQueue(c_info);
                return;
            }

            c_info.status_code = "200";
            c_info.r_contentType = contentType;
            readyQueue(c_info);

            //std::cout << c_info.status_code << ", type = " << c_info.r_contentType << std::endl;

        // 404 not found
        } else {
            c_info.status_code = "404";
            readyQueue(c_info);
            return;
        }
    // 400 bad request 
    } else {
        c_info.status_code = "400";
        readyQueue(c_info);
    }
}

void Parser::readyQueue(clientInfo c_info) {
	pthread_mutex_lock(&rqueue_lock);

    std::cout << "readyQueue: time = " << c_info.r_requestTime << std::endl;
    
	clientList.push_back(c_info);
	pthread_cond_signal(&rqueue_cond);
	pthread_mutex_unlock(&rqueue_lock);
}

// In this scheduler thread will fetch the request from the queue based on the scheduling policies
// For scheduler thread, the policy would be first come first serve (FCFS)
void Parser::popRequest() {
	sleep(s_time);
	while(1) {
		clientInfo c_info;
        pthread_mutex_lock(&rqueue_lock);
        while(clientList.empty())
            pthread_cond_wait(&rqueue_cond, &rqueue_lock);

        
        c_info = clientList.front();

        std::cout << "popRequest: time = " << c_info.r_requestTime << std::endl;

        //std::string header = 
        //    "HTTP/1.0 403 Forbidden\r\n"
        //    "Content-type: text/html\r\n"
        //    "Content-length: 151\r\n"
        //    "Date: \r\n\r\n";

        //std::string htmlBody = 
        //    "<html><head><title>403: Forbidden</title></head>\r\n"
        //    "<body><h1>Forbidden</h1><p>You do not not have permission to view this resource</p></body>\r\n</html>\r\n";

        //int a = write(c_info.r_acceptid, header.c_str(), header.size());
        //a += write(c_info.r_acceptid, htmlBody.c_str(), htmlBody.size());
        //std::cout << a << std::endl;


        clientList.pop_front();
        pthread_mutex_unlock(&rqueue_lock);
		
		pthread_mutex_lock(&send_lock);
		requestList.push_back(c_info);
		pthread_cond_signal(&send_cond);
		pthread_mutex_unlock(&send_lock);
	}
}

void *Parser::popRequest_helper(void *c) {
	Parser *P =(Parser *)c;
	P->popRequest();
	//((Parser *)c)->popRequest();
	//delete P;
	return NULL;
}

void *Parser::serveRequest_helper(void *c) {
    // copy the parser and call serve request since each of them operate independently
    Parser *P =(Parser *)c;
	P->serveRequest();
	return NULL;
}

void Parser::serveRequest()
{
	pthread_detach(pthread_self());
	while(1) {
		pthread_mutex_lock(&send_lock);
		
		while(requestList.empty())
				pthread_cond_wait(&send_cond, &send_lock);
		
		clientInfo c_info;

		c_info = requestList.front();
		requestList.pop_front();

        std::cout << "serveRequest: time = " << c_info.r_requestTime << std::endl;

        /* Generate Date for header*/
        char timestr[200];
        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        strftime(timestr, 200, "%a, %d %b %Y %H:%M:%S %Z", &tm);
        c_info.serveTime = timestr;
        pthread_mutex_unlock(&send_lock);

        sendData(c_info); 
    }
}
        

void Parser::sendData(clientInfo c_info) {
    std::string header;
    std::string htmlBody;
    std::ostringstream header_oss;
    std::ostringstream htmlBody_oss;

    if (c_info.status_code == "200") {
        std::string fullPath = c_info.r_filepath + c_info.r_filename;
        fullPath = std::regex_replace(fullPath, std::regex("%20"), " ");
        std::cout << "file path: " << fullPath << std::endl;

        std::string temp = readFile(fullPath);
        htmlBody_oss << temp << "\n";
        htmlBody = htmlBody_oss.str();
        c_info.r_filesize = htmlBody.size();

        header_oss << c_info.r_httpVersion << " 200 Ok\r\nContent-type: " << c_info.r_contentType << "\r\nContent-length: " << c_info.r_filesize << "\r\nDate: " << c_info.serveTime << "\r\n\r\n\n";
        header = header_oss.str();

    } else if (c_info.status_code == "400") {
        header = 
            "HTTP/1.0 400 Bad Request\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 119\r\n"
            "Date: ";
        header += c_info.serveTime + "\r\n\r\n";

        htmlBody = 
            "<html><head><title>400: Bad Request</title></head>\r\n"
            "<body><h1>Bad request</h1><p>Bad HTTP request</p></body>\r\n</html>\r\n";
    } else if (c_info.status_code == "404") {
        header = 
            "HTTP/1.0 404 Not Found\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 122\r\n"
            "Date: ";
        header += c_info.serveTime + "\r\n\r\n";

        htmlBody = 
            "<html><head><title>404: Not Found</title></head>\r\n"
            "<body><h1>Not Found</h1><p>Requested URL not found</p></body>\r\n</html>\r\n";
    } else {
        header = 
            "HTTP/1.0 403 Forbidden\r\n"
            "Content-type: text/html\r\n"
            "Content-length: 151\r\n"
            "Date: ";
        header += c_info.serveTime + "\r\n\r\n";

        htmlBody = 
            "<html><head><title>403: Forbidden</title></head>\r\n"
            "<body><h1>Forbidden</h1><p>You do not not have permission to view this resource</p></body>\r\n</html>\r\n";
    }
    
    std::cout << header << std::endl;
    std::cout << htmlBody << std::endl;

    // send response
    long bytesSent;
    bytesSent = write(c_info.r_acceptid, header.c_str(), header.size());
    bytesSent += write(c_info.r_acceptid, htmlBody.c_str(), htmlBody.size());
    if (bytesSent == header.size() + htmlBody.size()) {
        log("------ Server Response sent to client ------\n\n");
    } else {
        log("Error sending response to client");
    }

    close(c_info.r_acceptid);
}


std::string Parser::readFile(std::string path) {
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

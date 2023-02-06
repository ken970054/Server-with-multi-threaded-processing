#include "http_tcpServer_linux.h"
#include <iostream>
#include <string>
#include <sys/stat.h>

std::string document_root;

int main(int argc, char** argv) {
    /* variables */
    int portno;
    struct stat sb;

    /* check command line arguments */
    if (argc != 5) {
        std::cerr << "usage: " << argv[0] << " -document_root \"path/to/folder\" -port <portno>" << std::endl;
        exit(1);
    }
    /* read document_root path*/
    std::string s_argv1 = argv[1];
    if (s_argv1.compare("-document_root") == 0 && stat(argv[2], &sb) == 0) {
        document_root = argv[2];
    } else {
        std::cerr << "usage: " << argv[0] << " -document_root \"path/to/folder\" -port <portno>" << std::endl;
        std::cerr << "Please also make sure the root path is valid: check with pwd" << std::endl;
        exit(1);
    }

    /* read port number */
    std::string s_argv3 = argv[3];
    if (s_argv3.compare("-port") == 0){
        portno = atoi(argv[4]);
    } else {
        std::cerr << "usage: " << argv[0] << " -document_root \"path/to/folder\" -port <portno>" << std::endl;
        exit(1);
    }

    using namespace http;
    TcpServer server = TcpServer("127.0.0.1", portno, document_root);
    server.startListen();
    return 0;
}

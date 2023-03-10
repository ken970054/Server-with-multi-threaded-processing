#include "http_tcpServer_linux.h"
#include "parse_clientRequest.h"
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <pthread.h>

//#define BUFFER_SIZE 4096
std::string document_root;
int BUFFER_SIZE = 4096;
int s_time = 30;
int threadnum = 10;

Parser *P = new Parser();
pthread_mutex_t rqueue_lock;
pthread_cond_t rqueue_cond;
pthread_mutex_t send_lock;
pthread_cond_t send_cond;
pthread_t thread_scheduler;
pthread_t threads[30];

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


    pthread_mutex_init(&rqueue_lock, NULL);
    pthread_mutex_init(&send_lock, NULL);
    pthread_cond_init (&rqueue_cond, NULL);
    pthread_cond_init (&send_cond, NULL);
    pthread_create(&thread_scheduler, NULL, Parser::popRequest_helper, P);

    for(int i = 0; i < threadnum ; i++) pthread_create(&threads[i], NULL, Parser::serveRequest_helper, P);

    TcpServer server = TcpServer("127.0.0.1", portno, document_root);
    server.startListen(P);

    pthread_mutex_destroy(&rqueue_lock);
    pthread_cond_destroy(&rqueue_cond);
    pthread_mutex_destroy(&send_lock);
    pthread_cond_destroy(&send_cond);
    pthread_exit(NULL);

    return 0;
}

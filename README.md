# Server-with-multi-threaded-processing

Hao-Min Lin  
COEN 317 Distributed Systems Programming Assignment 1  
Due date: 2/7/23

### Description
Basic operation of the server will have a socket that
1. listens for incoming network connections
2. accepts a network connection from client request one at a time
3. reads the data (Request) sent from the client over the network connection
4. sends data (Response) to the client over the network connection
Notice that when the client request "GET /index.html HTTP/1.x", the upcoming request will have more than one time since there are image, css and js files in index.html that each can launch another new request. Therefore, the server has to process different types of request and send the write format of data back to client.

For multi-threaded processing, this design adopts first come first served (FCFS) policy to deal with multiple clients and multiple requests. There will be one main thread, one scheduler thread, and a threadpool of 10 threads (can adjust the number).  
Once we accept the request from client, the client will be pushed into a *client queue* if the request has succeeded. And if there is any client in the *client queue*, the first arrive client will start to handle the requests from the client and they are scheduled by *request queue* who are processed by threads in the threadpool.

**Compile command:**  
```
g++ http_tcpServer_linux.cpp server_linux.cpp parse_clientRequest.cpp -o server_linux 
```
**Usage:**  
```
./server_linux -document_root </path/to/folder> -port 8080
```
The SCU homepage files are located in the 'scu_files' folder, and to access index.html by GET request.

### Submitted files
* README.txt
* http_tcpServer_linux.cpp
* http_tcpServer_linux.h
* parse_clientRequest.cpp
* parse_clientRequest.h
* server_linux.cpp
* demonstration

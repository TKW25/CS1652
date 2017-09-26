#include "minet_socket.h"
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100
#define BACKLOG 5 //Incoming connection queue size, only a suggestion it seems

int handle_connection(int sock);

int main(int argc, char * argv[]) {
    int server_port = -1;
    int rc          =  0;
    int sock        = -1;
    printf("Starting\n");

    /* parse command line args */
    if (argc != 3) {
	fprintf(stderr, "usage: http_server1 k|u port\n");
	exit(-1);
    }

    server_port = atoi(argv[2]);

    if (server_port < 1500) {
	fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
	exit(-1);
    }

    /* initialize and make socket */
    minet_init(MINET_KERNEL); 
    if((sock = minet_socket(SOCK_STREAM)) == -1){
	perror("Error creating socket");
	return 1;
    }

    /* set server address*/
    struct sockaddr_in ip4addr;
    memset(&ip4addr, 0, sizeof(ip4addr));
    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(server_port);
    //I'm not actually sure what we're suppose to set our addrss to
    inet_pton(AF_INET, "127.0.0.1", &ip4addr.sin_addr);

    /* bind listening socket */
    if(minet_bind(sock, &ip4addr) == -1){
	perror("Error binding socket, port probably in use");
	return 1;
    }

    /* start listening */
    if(minet_listen(sock, BACKLOG) == -1){
	perror("Too many connections");
	return 1;
    }

    /* connection handling loop: wait to accept connection */
    /* declare variables for loop */
    int connfd; //temporary socket descriptor
    struct sockaddr_in addrs; //temporary sockaddr_in
    fd_set readfds; 
    struct timeval tv; //timeout value
    tv.tv_sec = 10;
    tv.tv_usec = 50000;
    int max; //largest file descriptor, needed for select
    int rv; //return value for select
    int i; //for loop counter
    int max_connections = 100; //Maximum number of active connections
    int socketfds[max_connections]; //array holding socket file descriptors
    while (1) {
	/* create read list */
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	max = sock;
	
	for(i = 0; i < max_connections; i++){
	    connfd = socketfds[i];
	    
	    if(connfd > 0){
		FD_SET(connfd, &readfds);
	    }
	    
	    if(connfd > max){
		max = connfd;
	    }
	}
	/* do a select */
	rv = minet_select(max + 1, &readfds, NULL, NULL, &tv);
	/* process sockets that are ready */
	if(rv == -1){
	    perror("error in select");
	}
	else if(rv == 0){
	    printf("Timeout. No data received after 10.5 seconds\n");
	}
	else{
	    /* for the accept socket, add accepted connection to connections */
	    if(FD_ISSET(sock, &readfds)){
		for(i = 0; i < max_connections; i++){
		   if(socketfds[i] <= 0){
			socketfds[i] = minet_accept(sock, &addrs);
			break;
		   } 
		}
	    }
	    else{
		/* for a connection socket, handle the connection */
		for(i = 0; i < max_connections; i++){
		    if(FD_ISSET(socketfds[i], &readfds)){
			rc = handle_connection(socketfds[i]);
			socketfds[i] = 0;
		    }
		}		
	    }
	}
    }
}

int handle_connection(int sock) {
    bool ok = false;

    const char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
	"Content-type: text/plain\r\n"			\
	"Content-length: %d \r\n\r\n";
    
    const char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
	"Content-type: text/html\r\n\r\n"			\
	"<html><body bgColor=black text=white>\n"		\
	"<h2>404 FILE NOT FOUND</h2>\n"				\
	"</body></html>\n";
    
    /* first read loop -- get request and headers*/

    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/

    /* try opening the file */

    /* send response */
    if (ok) {
	/* send headers */
	
	/* send file */
	
    } else {
	// send error response
    }

    /* close socket and free space */
  
    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

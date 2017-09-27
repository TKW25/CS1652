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
    printf("Starting Server...\n");

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
    int max; //largest file descriptor, needed for select
    int rv; //return value for select
    int i; //for loop counter
    int max_connections = 100; //Maximum number of active connections
    int socketfds[100] = {0}; //array holding socket file descriptors
    while (1) {
	/* create read list */
	rv = 1;
	tv.tv_sec = 10;
	tv.tv_usec = 50000;
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
	rv = minet_select(max + 1, &readfds, NULL, NULL, NULL);
	/* process sockets that are ready */
	if(rv == -1){
	    perror("error in select");
	}
	else if(rv == 0){ //Not certain if we even need this but I wrote it just in case
	    printf("Timeout. No data received after 10.5 seconds\n");
	    for(i = 0; i < max_connections; i++){
		char const *msg = "Connection timeout.  Closing connection...\n";
	    	if(socketfds > 0){
		    minet_write(socketfds[i], strdup(msg), strlen(msg) * sizeof(char));
		    minet_close(socketfds[i]);
		    socketfds[i] = 0;
		}
	    }
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
		//No available sockets, connection refused?
	    }
	    else{
		/* for a connection socket, handle the connection */
		for(i = 0; i < max_connections; i++){
		    if(FD_ISSET(socketfds[i], &readfds)){
			printf("Handling Connection\n");
			rc = handle_connection(socketfds[i]);
			socketfds[i] = 0; //Connection closed in handle, so update our list
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
	"<h2>404 FILE NOT FOUND</h2>\n"
	"</body></html>\n";

    /* first read loop -- get request and headers*/
    char buffer[BUFSIZE];
    memset(buffer, '\0', BUFSIZE);
    //A second buffer to handle telnet and other high latency connections
    char b_buffer[BUFSIZE];
    memset(b_buffer, '\0', BUFSIZE)
;
    //minet_read returning 0 -> socket closed
    while(minet_read(sock, buffer, BUFSIZE - 1) != 0){
	//Check for blank line signifying connection closed
	if((strcmp(buffer, "\r\n") == 0) | (strcmp(buffer, "\n") == 0)) 
	    break;
	strcat(b_buffer, buffer);
	memset(buffer, '\0', BUFSIZE);
    }
    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/
    char *str = strtok(b_buffer, " /\n");
    char *file = NULL;

    /* Given our assumptions this loop shouldn't do anything other than
     * quickly grab the file name, but I figure a loop would be necessary
     * for more complex requests we may need to handle in the future */
    while(str != NULL){
	if(!strcmp(str, "GET")){
	    str = strtok(NULL, " /\n");
	    file = str;
	    break; //For now we don't care about anything else
	}
	else{ //Placeholder mostly for testing the loop's behavior
	   str = strtok(NULL, " /\n");
	}
    }

    /* try opening the file */
    FILE *fp = fopen(str, "r");
    long fsize;
    char *contents = NULL;
    if(fp){
	if(fseek(fp, 0L, SEEK_END) == 0){
	    fsize = ftell(fp);
	    if(fsize == -1){
		perror("Bad buffer size\n");
		minet_close(sock);
		return NULL;
	    }
	    
	    contents = (char *) malloc(sizeof(char) * (fsize + 1));

	    if(fseek(fp, 0L, SEEK_SET) != 0){
		perror("Couldn't return to file start.\n");
		minet_close(sock);
		return NULL;
	    }

	    fread(contents, sizeof(char), fsize, fp);

	    if(ferror(fp) != 0){
		perror("Error reading file");
		minet_close(sock);
		return NULL;
	    }
	    else{
		contents[fsize + 1] = '\0';
	    }

	    ok = true;
	}
	fclose(fp);
    }
    /* send response */
    if (ok) {
	/* send headers */
	char outp[strlen(ok_response_f)];
	sprintf(outp, ok_response_f, fsize); 
	minet_write(sock, outp, strlen(ok_response_f)*sizeof(char));	
	/* send file */
	minet_write(sock, contents, fsize + 1);
	
    } else {
	// send error response
	char outp[strlen(notok_response)];
	sprintf(outp, notok_response);
	minet_write(sock, outp, strlen(notok_response)*sizeof(char));
    }
    
    /* close socket and free space */
    minet_close(sock);
    free(contents);
    printf("Done with connection...\n");
    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>

#define BUFSIZE 1024

int main(int argc, char * argv[]) {

    char * server_name = NULL;
    int server_port    = -1;
    char * server_path = NULL;
    char * req         = NULL;
    bool ok            = false;

    struct hostent *host;
    struct sockaddr_in address;
    fd_set fdset;
    struct timeval timeout;
    char header[12];
    int response;
    int reader;
    char c[1];
    char block[4];

    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];

    req = (char *)malloc(strlen("GET %d HTTP/1.0\r\n\r\n") * sizeof(char)
			 + (strlen(server_path) * sizeof(char) ));
    /* initialize */
    if (toupper(*(argv[1])) == 'K') {
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') {
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
	exit(-1);
    }
    /* make socket */
    int fd = minet_socket(SOCK_STREAM);

    /* get host IP address  */
    /* Hint: use gethostbyname() */
    host = gethostbyname(server_name);
    if (host == NULL){
      fprintf(stderr, "host not found\n");
      exit(-1);
    }

    /* set address */
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(server_port);
    memcpy(&address.sin_addr.s_addr, host->h_addr, host->h_length);

    /* connect to the server socket */
    minet_connect(fd, (struct sockaddr_in *)&address);
<<<<<<< HEAD
=======
    if (minet_connect < 0){
      fprintf(stderr, "connect failed\n");
      exit(-1);
    }

>>>>>>> 62fe52b1f3ce1fda5091e2217d6636eded92aa14
    /* send request message */
    //sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);
    //minet_write(fd, req, strlen(req)*sizeof(char));
    if (server_path[0] == '/')
      sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);
    else
      sprintf(req, "GET /%s HTTP/1.0\r\n\r\n", server_path);
    minet_write(fd, req, strlen(req)*sizeof(char));

    timeout.tv_sec = 10;
    timeout.tv_usec = 1000000;

    /* wait till socket can be read. */
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    minet_select(fd + 1, &fdset, NULL, NULL, NULL);
    /* Hint: use select(), and ignore timeout for now. */
    /* first read loop -- read headers */
    minet_read(fd, header, 12);
    if (minet_read <= 0){
      fprintf(stderr, "couldn't read header");
      exit(-1);
    }
    response = atoi(header + 9);
    /* examine return code */
    if (response == 200){     // Normal reply has return code 200
<<<<<<< HEAD
	printf("%s", header);	
	/*do{
        read_header = minet_read(fd, c, 1);
	if (strcmp(c, "\r") == 0){
=======
      do{
        reader = read(fd, c, 1);

        if (*c == '\r'){
>>>>>>> 62fe52b1f3ce1fda5091e2217d6636eded92aa14
          minet_read(fd, block, 3);
          if (minet_read < 0){
            fprintf(stderr, "Could not read block in header");
            exit(-1);
          }
          block[3] = '\0';
<<<<<<< HEAD
	  printf("this doesnt run does it\n");
          if (strcmp(block, "\n\r\n") == 0)
            break;
        }
      } while(read_header > 0); */
=======

          if (strcmp(block, "\n\r\n") == 0)
            break;
        }
      } while(reader > 0);
>>>>>>> 62fe52b1f3ce1fda5091e2217d6636eded92aa14
      do {
        reader = minet_read(fd, c, 1);
        if (reader > 0)
          printf("%c", c[0]);
      } while(reader > 0);
    }
    else {
      printf("%s", header);
      do{
        reader = minet_read(fd, c, 1);
        if (read > 0)
          printf("%c", c[0]);
      } while(reader > 0);
    }
    minet_close(fd);
    free(req);
    /*close socket and deinitialize */

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

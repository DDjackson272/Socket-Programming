/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "80" // the port client will be connecting to 

#define MAXDATASIZE 2000 // max number of bytes we can get at once 

//// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	int temp_recv = 0;
	//int buf_len = 0;
	//char * tempstr = NULL;
	FILE * output;
	char * token;
	char * token1;
	char * token2;
	char * port = PORT;
	char * domain;
	char * file_path;
	int data_length = 0;

	if (argc != 2) {
	    fprintf(stderr,"usage: ./http_client http://hostname[:port]/path/to/file\n");
	    exit(1);
	}

	token1 = strstr(argv[1], "http://"); // parse out http://
	if (token1 == NULL){ // make sure path user input has a "http://" header
	    fprintf(stderr,"usage: ./http_client http://hostname[:port]/path/to/file\n");
	    exit(1);
	}
	token1 += 7;
	token = strtok(token1, ":"); // parse out portnumber
	token = strtok(NULL, ":"); // parse out portnumber
	if (token != NULL){ // there is a port number defined by user
		domain = token1;
		token2 = strtok(token, "/");
		token2 = strtok(NULL, "\n");
		port = token;
		file_path = token2;
	} else { // there is not port number defined by user, use default one.
		token2 = strtok(token1, "/");
		token2 = strtok(NULL, "\n");
		domain = token1;
		file_path = token2;
	}

	// printf("domain: %s\n", domain);
	// printf("port: %s\n", port);
	// printf("file path: %s\n", file_path);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	if ((rv = getaddrinfo(domain, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	// int sockfd = socket(int domain(ipv4), int type(sock_stream-tcp, sock_dream-udp), 
	// int protocol(0-default for tcp))
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		//fprintf(stderr, "client: failed to connect\n");
		return 2;
	}


	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

 	memset(buf, '\0', sizeof buf); //clear buffer
	sprintf(buf, "GET /%s HTTP/1.0\r\n\r\n", file_path);
    //printf("%d\n", strcmp(port, "80"));
    //printf("%s", buf);
    send(sockfd, buf, strlen(buf),0);

    //printf("GET Sent...\n");
    memset(buf, '\0', sizeof buf);
    temp_recv = recv(sockfd,buf,MAXDATASIZE-1,0);
    output = fopen ("output","wb");
    //buf[temp_recv] = '\0';
    int count = 0;
    while (temp_recv > 0) {
    	//printf("\n==============================\n");
    	//printf("%s", buf);
    	if (count == 0){
    		char * content = strstr(buf, "\r\n\r\n");
    		if (content != NULL){
    			content += 4;
    		} else {
    			content = buf;
    		}
    		//fprintf (output, "%s", content);
    		fwrite(content, sizeof(char), strlen(content), output);
    		data_length += strlen(content);
    	} else {
    		//fprintf (output, "%s", buf);
    		fwrite(buf, sizeof(char), temp_recv, output);
    		data_length += temp_recv;
    	}
    	memset(buf, '\0', sizeof buf);
    	temp_recv = recv(sockfd,buf,MAXDATASIZE-1,0);
    	//buf[temp_recv] = '\0';
    	count += 1;
    	//printf("%d\n", temp_recv);
    	//printf("\n==============================\n");
    }

    
	//printf("Data acquired, the size is %d.\n", data_length);
	 
    
   	fclose (output);

	close(sockfd);

	return 0;
}


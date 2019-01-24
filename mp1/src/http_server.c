/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> // provide params for sys/socket.h and netinet/in.h
#include <sys/socket.h>// includes a number of definitions of structures needed for sockets
#include <netinet/in.h>// contains constants and structures needed for internet domain address
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3456"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 2000 // define MAXDATASIZE

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// check if file exists
int exists(const char *fname)
{
    FILE *file;
    if ((file = fopen(fname, "rb")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

// to concat two strings
char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); 
    strcpy(result, s1);
    strcat(result, s2);
    result[strlen(s1) + strlen(s2)] = '\0';
    return result;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// send all data
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sockfd, new connection on new_fd
	// addrinfo is one of the first thing you'll call when making a connection
	struct addrinfo hints, *servinfo, *p;
	// sockaddr_storage, similar t sockaddr except with larger space to store
	// cast to sockaddr when used
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char command[300];
	int command_len;
	FILE * target;
	char buf[MAXDATASIZE];
	//int buf_len;
	char * token;
	char * token1;
	char * token2;
	// char * token3;
	char * notFound = "HTTP/1.1 404 Not Found\r\n\r\n";
	char * Found = "HTTP/1.1 200 OK\r\n\r\n";
	char * badRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
	int data_length = 0;
	int file_length = 0;

	if (argc != 2) {
	    fprintf(stderr,"usage: ./http_client port\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	// int getaddrinfo(const char *node,          // host name to connect to, "www.google.com" or an IP address
	// 				const char *service,          // service like "http" or "ftp", or port number
	// 				const struct addrinfo *hints, // points to a struct addrinfo you've already filled with relevant info
	// 				struct addrinfo **res)        // result, gives you a pointer to a linked-list of results

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// sockfd is the socket file descriptor returned by socket().
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		// bind = 0 success, -1 failure
		// int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
		// bind socket to IP and PORT we defined
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		// printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			// receive command send from http_client
			command_len = recv(new_fd,command, sizeof(command)-1,0);
			command[command_len] = '\0';
			//printf("%s", command);

			// first validate the command, if it's invalid, send "HTTP/1.1 400 Bad Request"
			// otherwise send content required back

			// first check "GET" is valid or not
			token = strstr(command, "GET /");
			if (token != command){
				if (send(new_fd, badRequest, strlen(badRequest), 0) == -1)
					perror("send");
				exit(1);
			}
			// then check "HTTP/1.0" or "HTTP/1.1"
			token += 5;
			token1 = strtok(token, " "); //file name
			token2 = strtok(NULL, "\r\n"); // supposed to be "HTTP/1.0" or "HTTP/1.1"
			printf("%s\n", token1);
			printf("%s\n", token2);
			if (strcmp(token2, "HTTP/1.1") > 0){
				if (send(new_fd, badRequest, strlen(badRequest), 0) == -1)
					perror("send");
				exit(1);
			}

			// the request is valid, then start to send data required back

			// check if file exists, if exists, send content back to client
			if (exists(token1) == 1){
				target = fopen(token1,"rb");
				// first send http header
				if (send(new_fd, Found, strlen(Found), 0) == -1)
					perror("send");
				// then send content char by char
				size_t bytesRead = 0;
				while ((bytesRead = fread(buf, 1, sizeof(buf), target)) > 0){
  				  int temp = send(new_fd, buf, bytesRead, 0);
  				  if (temp == -1)
  				  	perror("send");
  				  data_length += temp;
  				}
				fclose(target);
			} else {
				if (send(new_fd, notFound, strlen(notFound), 0) == -1)
					perror("send");
			}

			//printf("Data sent, the size is %d.\n", data_length);
			close(new_fd);
			exit(0);
			
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}


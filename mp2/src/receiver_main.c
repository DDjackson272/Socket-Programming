/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <netdb.h>



#define RXBUFFERSIZE 200
#define MAXDATALENGTH 1472

struct sockaddr_in si_me, si_other;
int s;
int slen = sizeof si_other;
FILE * file;
const char *gotPktFrom;
char saddr[INET6_ADDRSTRLEN];
unsigned long long int totalPacketSize = 0;

struct Packet {
    char data[MAXDATALENGTH];
    long long int sequence;
    int bytesNumber;
};

// struct sockaddr_in {
//     short int          sin_family;  // Address family, AF_INET
//     unsigned short int sin_port;    // Port number
//     struct in_addr     sin_addr;    // Internet address
//     unsigned char      sin_zero[8]; // Same size as struct sockaddr
// };

void diep(char *s) {
    perror(s);
    exit(1);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void reliablyReceive(char *myUDPport, char *destinationFile) {
    
    //struct Packet* rx_arr = (struct Packet*) malloc(RXBUFFERSIZE * sizeof(struct Packet));
    long long int last_ack_pkt = -1;
    long long int exp_ack_pkt = 0;
    long long int totalBytesCount = 0;
    unsigned long long int numbytes = 0;
    //int rv;
    file = fopen(destinationFile, "wb");
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    getaddrinfo(NULL, myUDPport, &hints, &servinfo);

    s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    bind(s, servinfo->ai_addr, servinfo->ai_addrlen);

    freeaddrinfo(servinfo);

    /* Now receive data and send acknowledgements */ 

    printf("listener: waiting to recvfrom...\n");

    struct Packet *recvpkt = (struct Packet*) malloc (sizeof (struct Packet));

    //first recv a start pkt from sender and find the ip address from that start packet
    //then send ack for that start pkt.
    numbytes = recvfrom(s, recvpkt, sizeof (struct Packet), 0, 
        (struct sockaddr *)&si_me, (socklen_t *) &slen);

    while (strncmp(recvpkt->data, "STARTED", strlen("STARTED")) != 0){
        recvfrom(s, recvpkt, sizeof (struct Packet), 0, 
            (struct sockaddr *)&si_me, (socklen_t *) &slen);
    }

    totalPacketSize = recvpkt->bytesNumber;

    printf("totalPacketSize: %lld\n", totalPacketSize);

    //struct Packet *pktarray = (struct Packet*) malloc (totalPacketSize * sizeof (struct Packet));


    //find the IP address of pkt comes from
    gotPktFrom = inet_ntop(si_me.sin_family,
                    get_in_addr((struct sockaddr *)&si_me),
                        saddr, sizeof saddr);

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons((unsigned short int) atoi(myUDPport));
    if (inet_aton(gotPktFrom, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    //send ack for that start pkt.
    sendto(s, &(recvpkt->sequence), sizeof (long long int), 0, 
            (struct sockaddr *)&si_other, slen);

    //then start to recv real data
    numbytes = recvfrom(s, recvpkt, sizeof (struct Packet), 0, 
        (struct sockaddr *)&si_me, (socklen_t *) &slen);

    while (numbytes > 0 || recvpkt->sequence >= -1){
        //printf("%lldth pkt:\n", recvpkt->sequence);
        //printf("%s\n", recvpkt->data);
        long long int recv_seq = recvpkt -> sequence;
        if (recvpkt->bytesNumber != 0){
            //printf("In here\n");
            if (recv_seq == exp_ack_pkt){ 
                // if there is no pkt loss, then write data of pkt directly to the file 
                //and increase exp_ack_pkt.
                fwrite(recvpkt -> data, sizeof(char), recvpkt->bytesNumber, file);
                totalBytesCount += recvpkt->bytesNumber;
                //printf("%s\n", recvpkt->data);
                exp_ack_pkt = recv_seq+1;
                //rxWinBase++;
                last_ack_pkt = recv_seq;
            }
        }

        printf("expecting %lldth data,", exp_ack_pkt);
        printf("last acked %lldth data\n", last_ack_pkt);
        if(exp_ack_pkt >= totalPacketSize)
            break;

        //send ack back
        sendto(s, &last_ack_pkt, sizeof (long long int), 0, 
            (struct sockaddr *)&si_other, slen);

        // start to recvfrom again ...
        numbytes = recvfrom(s, recvpkt, sizeof (struct Packet), 0, 
            (struct sockaddr *)&si_me, (socklen_t *)&slen);
    }

    close(s);
    fclose(file);
    printf("%s received.\n", destinationFile);
    printf("total %lld bytes.\n", totalBytesCount);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    //unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    //udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(argv[1], argv[2]);

}
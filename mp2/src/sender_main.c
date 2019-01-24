/* 
 * File:   sender_main.c
 * Author:  Hengzhe Ding
 *          Dong Liu
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h> 

struct sockaddr_in si_other;
int s;
int slen = sizeof (si_other);
long long int last_ack_pkt = -10086;
long long int prev_ack = -10086;
long long int exp_ack_seq = 0;
int dupACKcount = 0;
unsigned long long int totalPacketSize = 0;
char * portChar = NULL;
struct sockaddr_storage their_addr;
socklen_t addr_len = sizeof their_addr;
unsigned long long int pointer = 0;
int count = 0;
int end = 1;
double cw = 1.0;
clock_t start_t, end_t;
int SST = 256;


#define MAXDATALENGTH 1472
#define RXBUFFERSIZE 200
#define TIMEOUT 30000


struct Packet {
    char data[MAXDATALENGTH];
    long long int sequence;
    int bytesNumber;
};

struct Packet finPacket = {"FINISHED", -2, strlen("FINISHED")};
struct Packet startPacket = {"STARTED", -1, strlen("STARTED")};


void diep(char *s) {
    perror(s);
    exit(1);
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, 
    unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    struct addrinfo hints, *res;

    fp = fopen(filename, "rb");  

    totalPacketSize = bytesToTransfer/MAXDATALENGTH;
    if (totalPacketSize*MAXDATALENGTH < bytesToTransfer){
        totalPacketSize++;
    } 

    double duration;
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;

    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, portChar, &hints, &res);

	/* Determine how many bytes to transfer */
    //build socket
    if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
        diep("socket");

    //bind socket with its port
    if (bind(s, res->ai_addr, res->ai_addrlen) == -1){
        perror("listener: bind");
    }

    //set timeout
    setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout));

    //set terminal
    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

	/* Send data and receive acknowledgements on s*/
    //struct Packet* pktarray = parseFileAndMakePkt(fp, bytesToTransfer);

    //When starting send data, first send startPkt to recver
    startPacket.bytesNumber = totalPacketSize;
    sendto(s, &startPacket, sizeof(struct Packet), 0, 
            (struct sockaddr *)&si_other, slen);

    //recv ack from recver on startPacket
    while (last_ack_pkt < -1){
        start_t = clock();
        recvfrom(s, &last_ack_pkt, sizeof(long long int), 0,
                (struct sockaddr *)&their_addr, &addr_len);
        end_t = clock();
        duration = end_t - start_t;
        if (duration >= TIMEOUT/1000){
            sendto(s, &startPacket, sizeof(struct Packet), 0, 
                (struct sockaddr *)&si_other, slen);
        }
    }

    printf("Start transmitting!\n");

    int count = 0;
    while (pointer < totalPacketSize && last_ack_pkt >= -1){
        //printf("%dth iteration.", count);
        //printf("With congestion window as %.3f.\n", cw);
        struct Packet *txWindow = (struct Packet *) malloc ((int)cw * sizeof (struct Packet));
        // make pkt and load into tx_buffer
        long long int pointer2 = pointer;
        while (pointer < end && pointer < totalPacketSize) {
            struct Packet *temp_pkt = (struct Packet*) malloc (sizeof (struct Packet));
            fseek(fp, pointer*MAXDATALENGTH, 0);
            memset(&temp_pkt->data, '\0', sizeof(temp_pkt->data));
            fseek(fp, pointer*MAXDATALENGTH, 0);
            int bytesRead;
            int totalBytesLeft = bytesToTransfer - pointer*MAXDATALENGTH;
            if (totalBytesLeft >= sizeof(temp_pkt->data)){
                bytesRead = fread(temp_pkt->data, 1, sizeof(temp_pkt->data), fp);
            } else {
                bytesRead = fread(temp_pkt->data, 1, totalBytesLeft, fp);
            }
            temp_pkt->sequence = pointer;
            temp_pkt->bytesNumber = bytesRead;
            txWindow[pointer % ((int)cw)] = *temp_pkt;
            pointer += 1;
        }

        // send pkt in tx_buffer
        pointer = pointer2;
        while (pointer < end){
            struct Packet *pktToSend = &(txWindow[pointer % ((int)cw)]);
            sendto(s, pktToSend, sizeof(struct Packet), 0, 
                    (struct sockaddr *)&si_other, slen);
            pointer += 1;
        }

        prev_ack = -3;
        int recv_byte = recvfrom(s, &last_ack_pkt, sizeof (long long int), 0, 
                (struct sockaddr *)&their_addr, &addr_len);
        
        while (recv_byte > 0){
            if (prev_ack == last_ack_pkt){
                //printf("Encounter dupACK: %lldth pkt.\n", prev_ack);
                dupACKcount ++;
                if (dupACKcount == 3){
                    cw = (double)((cw/2) + 3);
                    SST = (int)cw/2;
                } else if (dupACKcount > 3){
                    cw += 1.0;
                }
            } else {
                if (dupACKcount > 0){
                    dupACKcount = 0;
                    cw = (double) SST;
                }
                if (cw < SST){
                    cw += 1.0;
                } else {
                    cw += (double)(1/cw);
                }
            }

            pointer = last_ack_pkt + 1;
            prev_ack = last_ack_pkt;
            recv_byte = recvfrom(s, &last_ack_pkt, sizeof (long long int), 0, 
                            (struct sockaddr *)&their_addr, &addr_len);
        }
        count += 1;
        end = pointer + (int) cw;
    }



    printf("Closing the socket\n");
    close(s);
    fclose(fp);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;
    time_t t1, t2;
    time(&t1);


    if (argc != 5) {
        fprintf(stderr, 
            "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", 
            argv[0]);
        exit(1);
    }
    portChar = argv[2];
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    //sleep(10);
    time(&t2);
    //free(portChar);
    printf("Time consumption: %lds.\n", t2-t1);
    return (EXIT_SUCCESS);
}



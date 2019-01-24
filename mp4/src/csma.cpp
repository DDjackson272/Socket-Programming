#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <iostream> 
#include <unordered_map>
#include <vector>
#include <set> 
#include <iterator>
#include <map> 
using namespace std; 

struct node {
	int backoff;
	int packetSent;
	int currBackoffWindowPos;
	int collidedTime;
	int reTxCnt;
};

FILE *inputFile;
FILE *outputFile = fopen("output.txt", "wb");
//FILE *nodeFile = fopen("node.txt", "wb");

int global_time = 0;
bool channelOccupied = false;
int nodeNumber = 0;
int packetLength = 0;
int randomNumber[20];
int maxCount = 0;
int transmissionTime = 0;
int iter_counter = 0;
node* allNode = NULL;
int totalTxedPkt = 0;
int fullUtilization = 0;
int idleTime = 0;
int busyTime = 0;
int totalNodeCollision = 0;
int totalCollision = 0;
float avgPktSentByNode = 0.0;
float varPktSentByNode = 0.0;
float avgCollision = 0.0;
float varCollision = 0.0;

void readInputFile(){
    char* line = NULL;
    size_t len = 0;
    memset(randomNumber, 0, sizeof(int));
    while ((getline(&line, &len, inputFile)) != -1){
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        char* notation = strtok(line, " ");
        if (strcmp(notation, "N") == 0)
            nodeNumber = atoi(strtok(NULL, " "));
        else if (strcmp(notation, "L") == 0)
            packetLength = atoi(strtok(NULL, " "));
        else if (strcmp(notation, "M") == 0)
            maxCount = atoi(strtok(NULL, " "));
        else if (strcmp(notation, "T") == 0)
            transmissionTime = atoi(strtok(NULL, " "));
        else {
            char* temp = strtok(NULL, " ");
            while (temp){
                randomNumber[iter_counter] = atoi(temp);
                iter_counter += 1;
                temp = strtok(NULL, " ");
            }
        }
    }
}

void writeOutputFile(){
    char temp[100];

    memset(temp, '\0', sizeof(char));
    sprintf(temp, "Channel utilization (in percentage) %0.2f %%\n", float(busyTime)*100.0/transmissionTime);
    // printf("Channel utilization (in percentage) %0.2f %%\n", float(busyTime)*100.0/transmissionTime);
    fwrite(temp, sizeof(char), strlen(temp), outputFile);

    memset(temp, '\0', sizeof(char));
    sprintf(temp, "Channel idle fraction (in percentage) %0.2f %%\n", float(idleTime)*100.0/transmissionTime);
    fwrite(temp, sizeof(char), strlen(temp), outputFile);

    memset(temp, '\0', sizeof(char));
    sprintf(temp, "Total number of collisions %d\n", totalCollision);
    fwrite(temp, sizeof(char), strlen(temp), outputFile);

    memset(temp, '\0', sizeof(char));
    for(int i = 0; i < nodeNumber; ++i){
        avgPktSentByNode += float(allNode[i].packetSent)/nodeNumber;
    }
    for(int i = 0; i < nodeNumber; ++i){
        varPktSentByNode +=
                ((allNode[i].packetSent-avgPktSentByNode)*(allNode[i].packetSent-avgPktSentByNode))/nodeNumber;
    }
    sprintf(temp, "Variance in number of successful transmissions (across all nodes) %0.2f\n",
            varPktSentByNode);
    fwrite(temp, sizeof(char), strlen(temp), outputFile);

    memset(temp, '\0', sizeof(char));
    avgCollision = totalNodeCollision/nodeNumber;
    for(int i = 0; i < nodeNumber; ++i){
        varCollision += ((allNode[i].collidedTime - avgCollision)*
                (allNode[i].collidedTime - avgCollision))/nodeNumber;
    }
    sprintf(temp, "Variance in number of collisions (across all nodes) %0.2f\n", varCollision);
    fwrite(temp, sizeof(char), strlen(temp), outputFile);
}

//void printNodeStatus(){
//    for(int i = 0; i < nodeNumber; ++i){
//        char temp[500];
//        memset(temp, '\0', sizeof(char));
//        sprintf(temp, "Node id: %d, backoff time: %d, packet sent: %d, collided time: %d, reTxCount: %d\n",
//                i, allNode[i].backoff, allNode[i].packetSent, allNode[i].collidedTime, allNode[i].reTxCnt);
//        fwrite(temp, sizeof(char), strlen(temp), nodeFile);
//    }
//}

void verifyInput(){
    printf("Node number is: %d\n", nodeNumber);
    printf("Packet length is: %d\n", packetLength);
    printf("Random numbers are: ");
    for (int i = 0; i < iter_counter; ++i){
        printf("%d ", randomNumber[i]);
    }
    printf("\n");
    printf("Max count is: %d\n", maxCount);
    printf("Transmission time is: %d\n", transmissionTime);
}

void verifyNodes(){
    for(int i = 0; i < nodeNumber; ++i){
        printf("Node id: %d, backoff time: %d\n", i, allNode[i].backoff);
    }
}

void createNodes(){
    allNode = (node*) malloc (sizeof(struct node) * nodeNumber);
    for (int i = 0; i < nodeNumber; ++i){
        node* temp = (node*) malloc (sizeof(struct node));
        temp->currBackoffWindowPos = 0;
        temp->backoff = rand() % (randomNumber[temp->currBackoffWindowPos]+1) + 1;
        temp->packetSent = 0;
        temp->collidedTime = 0;
        temp->reTxCnt = 0;
        allNode[i] = *temp;
    }
}

void setFlag(){
    int* readyToTx = (int*) malloc (nodeNumber * sizeof(int));
    memset(readyToTx, -1, sizeof(int));
    int iter_ptr = 0;
    for (int i = 0; i < nodeNumber; ++i){
        if (allNode[i].backoff == 0){
            readyToTx[iter_ptr] = i;
            iter_ptr += 1;
        }
    }
    if(iter_ptr > 1){
        totalCollision += 1;
        for(int i = 0; i < iter_ptr; ++i){
            int nodePos = readyToTx[i];
            allNode[nodePos].currBackoffWindowPos += 1;
            if (allNode[nodePos].currBackoffWindowPos >= iter_counter){
                allNode[nodePos].currBackoffWindowPos = iter_counter-1;
            }
            allNode[nodePos].collidedTime += 1;
            allNode[nodePos].reTxCnt += 1;
            if(allNode[nodePos].reTxCnt >= maxCount){
                allNode[nodePos].reTxCnt = 0;
                allNode[nodePos].currBackoffWindowPos = 0;
            }
            allNode[nodePos].backoff = rand() % (randomNumber[allNode[nodePos].currBackoffWindowPos]+1)+1;
        }
    } else if (iter_ptr == 1){
        int nodePos = readyToTx[0];
        if(allNode[nodePos].reTxCnt < maxCount){
            channelOccupied = true;
            allNode[nodePos].packetSent += 1;
            allNode[nodePos].currBackoffWindowPos = 0;
        }
        allNode[nodePos].backoff = rand() % (randomNumber[allNode[nodePos].currBackoffWindowPos]+1)+1;
        allNode[nodePos].reTxCnt = 0;
    }
}

void countDown(){
    setFlag();
    if (!channelOccupied){
        for(int i = 0; i < nodeNumber; ++i){
            allNode[i].backoff -= 1;
        }
    }
}

void transmission(){
    int t = 0;
    while(t < transmissionTime){
//        char temp[50];
//        memset(temp, '\0', sizeof(char));
//        sprintf(temp, "t = %d\n", t);
//        fwrite(temp, sizeof(char), strlen(temp), nodeFile);
//        printNodeStatus();
        countDown();
        if(channelOccupied){
            t += packetLength;
            busyTime += packetLength;
            channelOccupied = false;
        } else {
            t += 1;
        }
    }
}

void printFinalResult(){
    for (int i = 0; i < nodeNumber; ++ i){
        totalTxedPkt += allNode[i].packetSent;
        totalNodeCollision += allNode[i].collidedTime;
    }
    idleTime = transmissionTime - busyTime - totalCollision;
    printf("Total pkt sent: %d\n", totalTxedPkt);
}

void plotDataCollection(){
    float* channelUtilizationArray = (float *) malloc (100 * sizeof(float));
    float* channelIdleArray = (float *) malloc (100 * sizeof(float));
    int* totalCollisionArray = (int *) malloc (100 * sizeof(int));
    FILE * channelUtilizationFile = fopen("busy.csv", "wb");
    FILE * channelIdleFile = fopen("idle.csv", "wb");
    FILE * totalCollisionFile = fopen("collision.csv", "wb");
    for (int b = 5; b <= 500; b += 5){
        global_time = 0;
        channelOccupied = false;
        allNode = NULL;
        totalTxedPkt = 0;
        fullUtilization = 0;
        idleTime = 0;
        busyTime = 0;
        totalNodeCollision = 0;
        totalCollision = 0;
        avgPktSentByNode = 0.0;
        varPktSentByNode = 0.0;
        avgCollision = 0.0;
        varCollision = 0.0;

        nodeNumber = b;

        srand((unsigned)time(NULL));

        createNodes();

        transmission();

        printFinalResult();

        writeOutputFile();

        channelUtilizationArray[(b/5)-1] = float(busyTime)*100.0/transmissionTime;

        channelIdleArray[(b/5)-1] = float(idleTime)*100.0/transmissionTime;

        totalCollisionArray[(b/5)-1] = totalCollision;
    }

    for (int i = 0; i < 100; ++i){
        char per[20];
        memset(per, '\0', sizeof(char));
        if (i < 99){
            sprintf(per, "%0.2f,", channelUtilizationArray[i]);
            fwrite(per, sizeof(char), strlen(per), channelUtilizationFile);
        } else {
            sprintf(per, "%0.2f", channelUtilizationArray[i]);
            fwrite(per, sizeof(char), strlen(per), channelUtilizationFile);
        }

    }

    for (int i = 0; i < 100; ++i){
        char per[20];
        memset(per, '\0', sizeof(char));
        if (i < 99){
            sprintf(per, "%0.2f,", channelIdleArray[i]);
            fwrite(per, sizeof(char), strlen(per), channelIdleFile);
        } else {
            sprintf(per, "%0.2f", channelIdleArray[i]);
            fwrite(per, sizeof(char), strlen(per), channelIdleFile);
        }
    }

    for (int i = 0; i < 100; ++i){
        char per[20];
        memset(per, '\0', sizeof(char));
        if (i < 99){
            sprintf(per, "%d,", totalCollisionArray[i]);
            fwrite(per, sizeof(char), strlen(per), totalCollisionFile);
        } else {
            sprintf(per, "%d", totalCollisionArray[i]);
            fwrite(per, sizeof(char), strlen(per), totalCollisionFile);
        }
    }
}

void regularLaunch(){
    srand((unsigned)time(NULL));

    createNodes();

    transmission();

    printFinalResult();

    writeOutputFile();
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: ./csma inputfile\n");
        return -1;
    }

    inputFile = fopen(argv[1], "rb");

    readInputFile();

    regularLaunch();

    // plotDataCollection();
}


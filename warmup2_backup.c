/*
 * Author:      Moonsoo Jo (moonsooj@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
//must include file after putting it in 'make'
#include <pthread.h>

#include "cs402.h"

#include "my402list.h"


typedef struct packet {
    int tokensNeeded;
    int serviceTime;
} Packet;


static
void Usage()
{
    fprintf(stderr,
            "usage: %s\n",
            "./warmup2 ");
    exit(-1);
}

int number = 0;
char* token_bucket[100];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
My402List q1;
My402List q2;
/* global vars for testing */
int numToArrive = 20;
int lambda = 1;
double mu = 0.35;
double r = 1.5;
int B = 10;
int P = 3;
int tokens;
int n = 1;


void *PacketThread(void *);
void *TokenThread(void *);
void *ServerThread(void *);

/* ----------------------- main() ----------------------- */

//q1 -> when tokens arrive, it sleeps first
//when enough of them do, it gets put into q2

//sleep while transmitting

//thread that generates packets: packet thread
//thread that generates tokens: token thread
//threads that transmit packets: server thread 1 and server thread 2

//threads talk to each other through shared variables

//something takes x microseconds: usleep(x)
//if it takes longer than expected, you need to figure out if it's due to the OS's property or a bug in your code

//don't use "pthread_priority"

//output will be different in every simulation

//put anything used in q1 and q2 in the serialization box (one mutex) (e.g. stdout (printf?))

//pthread condition broadcast to wake the server threads up
//one of the server thread takes the mutex and removes the packet from q2
//server thread calls "free-on"

//response time: departure time (when the packet is transmitted from the server) - arrival time (when the server gets the packet)
//q1(time): time it takes for packet to leave q1
//q2(time): time it takes for packet to leave q2
//timestamp packet and calculate statistics

//not every token needs "b" amount of packets
//when bucket has b-1 amount of packets, when the next packet comes, it wakes up the threads so one of them can get the token
//if a server thread is sleeping, the other one will get it

//packet can move into q1, q2, and the servers

int GetNumLines(char *field, int field_sz) {
    char *start_ptr = field;
    char *newline_ptr = strchr(start_ptr, '\n');
    if (newline_ptr != NULL) {
        *newline_ptr++ = '\0';
    }
    strncpy(field, start_ptr, field_sz);
    field[field_sz-1] = '\0';
    int numLines;
    numLines = atoi(field);
    return numLines;
}

int main(int argc, char *argv[])
{
    
    memset(&q1, 0, sizeof(q1));
    if (!My402ListInit(&q1)) {
        printf("error: list not initialized\n");
    }

    //time of start
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    //current_time.tv_sec, current_time.tv_usec
    printf("00000000.000ms: emulation begins\n");

    //threads
    pthread_t packetThread;
    pthread_t tokenThread;
    pthread_t serverThread1;
    pthread_t serverThread2;

    int arg1 = 1;
    int arg2 = 2;
    int arg3 = 3;
    int argument = 1;
    int result;
    char packet[1];

    //take in input here
    //pass 3 arguments as a package into the threads
    pthread_create(&packetThread, 0, PacketThread, &packet);
    pthread_create(&tokenThread, 0, TokenThread, (void *)argument);
    pthread_create(&serverThread1, 0, ServerThread, (void *)arg1);
    pthread_create(&serverThread2, 0, ServerThread, (void *)arg2);

    pthread_join(packetThread, 0);
    pthread_join(tokenThread, 0);
    pthread_join(serverThread1, 0);
    pthread_join(serverThread2, 0);

    printf("statistics: <here>\n");

    return(0);
}

Packet *NewPacket(tokensNeeded, serviceTime) {
    Packet *packet = (Packet*)malloc(sizeof(Packet));
    packet -> tokensNeeded = tokensNeeded;
    packet -> serviceTime = serviceTime;
    return packet;
}

void *PacketThread(void *arg) {
    //for()
        //sleep
        //generate packet/token
        //add packet/token to token filter

    //open the file
    FILE *fp = fopen("tsfile.txt", "r");
    if (fp == NULL) {
        printf("failed to open the file\n");
        perror("");
        return 1;
    }

    //get the first line
    char buf[2000];
    fgets(buf, sizeof(buf), fp);
    char numLines[2000];
    strncpy(numLines, buf, sizeof(numLines));
    int numLinesInt = GetNumLines(numLines, sizeof(numLines));

    //parse the contentMy402ListAppend(&q1, packet);
    int interArrivalTime, tokensNeeded, serviceTime;
    //after total number of packets to generate,
    //terminate it
    for (int i = 0; i < numLinesInt; i++) {
        //get line
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            if (sscanf(buf, "%d %d %d", &interArrivalTime, &tokensNeeded, &serviceTime) == 3) {
                /* got 3 integers in a, b, and c */
                usleep(lambda * 1000);
                Packet *packet = NewPacket(tokensNeeded, serviceTime);
                pthread_mutex_lock(&mutex);

                /* do some stuff */
                //circular list
                if (My402ListEmpty(&q1) != true) {
                    My402ListAppend(&q1, packet);
                } else if (tokens >= tokensNeeded) {
                    //tokens -= tokensNeeded;
                    My402ListAppend(&q2, packet);
                    My402ListUnlink(&q1, My402ListFirst(&q1));
                    pthread_cond_broadcast(&cv);
                    tokens = 0;
                }
                printf("p%d arrives, needs %d tokens, inter-arrival time = %fms\n", i, tokensNeeded, (double) interArrivalTime / 1000);
                pthread_mutex_unlock(&mutex);
            }
        }
    }

    pthread_exit((void *)1);
    //return (0);

}

//generates tokens at a steady rate
//when enough token is generated
//transmit the packet into the server
void *TokenThread(void *arg) {
    //infinite loop
    int counter = 0;
    while (counter < 20) {
        counter++;
        
        //find out when the token was generated
        //add 1/r to it
        //that's how much you need to sleep
        
        pthread_mutex_lock(&mutex);
        tokens++;
        if (My402ListEmpty(&q1) != true) {
            Packet *firstPacket = My402ListFirst(&q1) -> obj;
            if (tokens >= firstPacket -> tokensNeeded) {
                My402ListAppend(&q2, My402ListFirst(&q1) -> obj);
                My402ListUnlink(&q1, My402ListFirst(&q1));
                pthread_cond_broadcast(&cv);
                tokens = 0;
            }
        }
        usleep(1/r * 1000);
        
        printf("%fms: token t1 arrives, token bucket now has %d token\n", 1/r * 1000, tokens);

        pthread_mutex_unlock(&mutex);
    }

    pthread_exit((void *)1);
}

void *ServerThread(void *arg) {
    //infinite loop
    //under what condition should i quit the server thread?
    int counter = 0;
    while (counter < 10) {
        counter++;
        pthread_mutex_lock(&mutex);
        //time_to_quit
        while (My402ListLength(&q2) == 0) {
            pthread_cond_wait(&cv, &mutex);
        }
        Packet *firstPacket = (My402ListFirst(&q2) -> obj);
        pthread_mutex_unlock(&mutex);
        //work
        usleep(firstPacket -> serviceTime);
        
    }
    return (0);

}
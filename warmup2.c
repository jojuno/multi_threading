/*
 * Author:      Moonsoo Jo (moonsooj@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
//must include file after putting it in 'make'
#include <pthread.h>

#include "cs402.h"

#include "my402list.h"


typedef struct packet {
    int tokensNeeded;
    double serviceTime;
    long int createdSec;
    long int createdMs;
} Packet;

typedef struct timePassed {
    long int timeSec;
    long int timeUsec;
} TimePassed;

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
int tokens = 0;
int tokens_total = 0;
int n = 1;
struct timeval current_time;
int packets = 0;

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

TimePassed GetTimePassed() {
    struct timeval creation_time, time_difference;
    gettimeofday(&creation_time, NULL);
    timersub(&creation_time, &current_time, &time_difference);

    //format time
    long int time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
    long int time_difference_long_int = (long int) time_difference_combined;
    long int time_difference_left_over = time_difference_combined - time_difference_long_int;
    long int time_difference_left_over_long_int = time_difference_left_over * 1000;

    //prints '000' for the decimals

    TimePassed t;
    t.timeSec = time_difference_long_int;
    t.timeUsec = time_difference_left_over_long_int;

    return t;

}

int main(int argc, char *argv[])
{
    
    memset(&q1, 0, sizeof(q1));
    if (!My402ListInit(&q1)) {
        printf("error: list not initialized\n");
    }

    //time of start
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

    //take in input here
    //pass 3 arguments as a package into the threads
    pthread_create(&packetThread, 0, PacketThread, (void *) arg3);
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

Packet *NewPacket(int tokensNeeded, int serviceTime, long int createdSec, long int createdMs) {
    Packet *packet = (Packet*)malloc(sizeof(Packet));
    packet -> tokensNeeded = tokensNeeded;
    packet -> serviceTime = serviceTime;
    packet -> createdSec = createdSec;
    packet -> createdMs = createdMs;
    return packet;
}

void *PacketThread(void *arg) {

    usleep(lambda * 1000000);
    
    //packet arrival time
    struct timeval packet_creation_time, time_difference;
    gettimeofday(&packet_creation_time, NULL);
    timersub(&packet_creation_time, &current_time, &time_difference);

    //format time
    double time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
    long int time_difference_long_int = (long int) time_difference_combined;
    double time_difference_left_over = time_difference_combined - time_difference_long_int;
    long int time_difference_left_over_long_int = time_difference_left_over * 1000;

    Packet *packet = NewPacket(P, 1/mu, time_difference_long_int, time_difference_left_over_long_int);
    packets++;
    pthread_mutex_lock(&mutex);

    printf(
        "%08ld.%03ldms: p%d arrives, needs %d tokens, inter-arrival time: %08ld.%03ldms\n", 
        time_difference_long_int, 
        time_difference_left_over_long_int, 
        tokens_total, 
        tokens,
        time_difference_long_int, 
        time_difference_left_over_long_int
    );

    if (tokens < P) {
        //printf("appending packet to q1\n");
        My402ListAppend(&q1, packet);
    }
    
    if (tokens >= P) {
        My402ListElem *elem = My402ListFirst(&q1);
        Packet *packet_first = elem -> obj;
        My402ListElem *new_elem = (My402ListElem*)malloc(sizeof(My402ListElem));
        new_elem -> obj = &packet_first;
        My402ListAppend(&q2, new_elem);
        My402ListUnlink(&q1, My402ListFirst(&q1));
        pthread_cond_broadcast(&cv);
        tokens = 0;
    }

    //packet arrival time
    gettimeofday(&packet_creation_time, NULL);
    timersub(&packet_creation_time, &current_time, &time_difference);

    //format time
    time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
    time_difference_long_int = (long int) time_difference_combined;
    time_difference_left_over = time_difference_combined - time_difference_long_int;
    time_difference_left_over_long_int = time_difference_left_over * 1000;

    printf(
        "%08ld.%03ldms: p%d enters Q1\n", 
        time_difference_long_int, 
        time_difference_left_over_long_int, 
        tokens_total
    );

    pthread_mutex_unlock(&mutex);

    return (0);

}

//generates tokens at a steady rate
//when enough token is generated
//transmit the packet into the server
void *TokenThread(void *arg) {
    //infinite loop
    for(;;) {
        usleep(1/r * 1000000);
        
        //find out when the token was generated
        //add 1/r to it
        //that's how much you need to sleep
        
        pthread_mutex_lock(&mutex);
        tokens++;
        tokens_total++;
        
        //token arrival time
        struct timeval token_creation_time, time_difference;
        gettimeofday(&token_creation_time, NULL);
        timersub(&token_creation_time, &current_time, &time_difference);

        double time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
        long int time_difference_long_int = (long int) time_difference_combined;
        double time_difference_left_over = time_difference_combined - time_difference_long_int;
        long int time_difference_left_over_long_int = time_difference_left_over * 1000;

        //pluralize "token"
        if (tokens > 1) {
            printf(
                "%08ld.%03ldms: token t%d arrives, token bucket now has %d tokens\n", 
                time_difference_long_int, 
                time_difference_left_over_long_int, 
                tokens_total, 
                tokens
            );
        } else {
            printf(
                "%08ld.%03ldms: token t%d arrives, token bucket now has %d token\n", 
                time_difference_long_int, 
                time_difference_left_over_long_int, 
                tokens_total, 
                tokens
            );
        }

        //check if there's enough tokens to remove a packet
        if (My402ListEmpty(&q1) != TRUE) {
            My402ListElem *firstElem = My402ListFirst(&q1);
            if (&firstElem != NULL) {}
            Packet *packet = firstElem -> obj;
            if (packet -> tokensNeeded <= tokens) {
                //00002128.261ms: p1 leaves Q1, time in Q1 = 1082.232ms, token bucket now has 0 token
                My402ListUnlink(&q1, firstElem);
                tokens = 0;

                //packet departure time
                struct timeval packet_departure_time, time_difference;
                gettimeofday(&packet_departure_time, NULL);
                timersub(&packet_departure_time, &current_time, &time_difference);

                //format time
                time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
                time_difference_long_int = (long int) time_difference_combined;
                time_difference_left_over = time_difference_combined - time_difference_long_int;
                time_difference_left_over_long_int = time_difference_left_over * 1000;

                //format packet's creation time
                long int time_difference_packet_long_int = time_difference_long_int - packet -> createdSec;
                long int time_difference_packet_left_over_long_int = time_difference_left_over_long_int - packet -> createdMs;
                if (time_difference_packet_left_over_long_int < 0) {
                    time_difference_packet_long_int--;
                    time_difference_packet_left_over_long_int = 1000 + time_difference_packet_left_over_long_int;
                }

                printf(
                    "%08ld.%03ldms: p%d leaves Q1, time in Q1 = %08ld.%03ldms, token bucket now has %d token\n", 
                    time_difference_long_int, 
                    time_difference_left_over_long_int, 
                    packets,
                    time_difference_packet_long_int,
                    time_difference_packet_left_over_long_int,
                    tokens
                );

                My402ListAppend(&q2, packet);

                //pick it up here

                TimePassed t = GetTimePassed();

                //00002129.187ms: p1 enters Q2
                printf(
                    "%08ld.%03ldms: p%d enters Q2\n", 
                    t.timeSec, 
                    t.timeUsec, 
                    packets
                );

            }
        }

        /*
            00002129.187ms: p1 enters Q2
            00002130.350ms: p1 leaves Q2, time in Q2 = 1.163ms
            //wake up thread
            00002130.817ms: p1 begins service at S2, requesting 2857ms of service
            00004991.834ms: p1 departs from S2, service time = 2861.017ms, time in system = 3945.962ms
            //server wakes up
            //next server sleeps because all packets have arrived, and q1 and q2 are empty
            00004992.989ms: emulation ends
        */



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
        //pthread_mutex_lock(&mutex);
        //time_to_quit
        while (My402ListLength(&q2) == 0) {
            //pthread_cond_wait(&cv, &mutex);
        }
        //Packet *firstPacket = (My402ListFirst(&q2) -> obj);
        //pthread_mutex_unlock(&mutex);
        //work
        //usleep(firstPacket -> serviceTime);
        
    }
    return (0);

}
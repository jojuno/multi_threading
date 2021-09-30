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
    long int q2EnteredSec;
    long int q2EnteredUsec;
    long int ogCreatedSec;
    long int ogCreatedUsec;
    double timeInSystem;
    int id;
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
int numToArrive = 1;
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
int numArrived = 0;
double interArrivalTimeTotal = 0;
double serviceTimeTotal = 0;
double q1TimeTotal = 0;
double q2TimeTotal = 0;
double s1TimeTotal = 0;
double s2TimeTotal = 0;
double totalPacketTime = 0;
double timeSpentInSystem = 0;
double totalEmulationTime = 0; //ms



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
    double time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
    long int time_difference_long_int = (long int) time_difference_combined;
    double time_difference_left_over = time_difference_combined - time_difference_long_int;
    long int time_difference_left_over_long_int = time_difference_left_over * 1000;

    //prints '000' for the decimals

    TimePassed t;
    t.timeSec = time_difference_long_int;
    t.timeUsec = time_difference_left_over_long_int;

    return t;

}

TimePassed GetTimePassed2(struct timeval t1, struct timeval t2) {
    struct timeval time_difference;
    timersub(&t1, &t2, &time_difference);

    //format time
    double time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
    long int time_difference_long_int = (long int) time_difference_combined;
    double time_difference_left_over = time_difference_combined - time_difference_long_int;
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

    //token arrival time
    struct timeval emulation_finished_time, emulation_finished_time_difference;
    gettimeofday(&emulation_finished_time, NULL);

    TimePassed emulation_finished_time_passed = GetTimePassed2(emulation_finished_time, current_time);
    printf(
        "%08ld.%03ldms: emulation ends\n", 
        emulation_finished_time_passed.timeSec, 
        emulation_finished_time_passed.timeUsec
    );
    timersub(&emulation_finished_time, &current_time, &emulation_finished_time_difference);
    totalEmulationTime = emulation_finished_time_difference.tv_sec * 1000 + (double) emulation_finished_time_difference.tv_usec / 1000;

    PrintStats();

    return(0);
}

void PrintStats(){
    //add them
    //divide by the number of packets
    //in seconds
    //6 digits of precision
    printf("\n");
    printf("Statistics:\n");
    printf("\n");

    printf("    average packet inter-arrival time = %.5f\n", interArrivalTimeTotal / packets / 1000);
    printf("    average packet service time = %.5f\n", serviceTimeTotal / packets / 1000);
    printf("\n");
    //divide number into whole number and decimal number
    printf("    average number of packets in Q1 = %.5f\n", q1TimeTotal / (double) totalEmulationTime);
    printf("    average number of packets in Q2 = %.6g\n", (double) q2TimeTotal / (double) totalEmulationTime);
    printf("    average number of packets at S1 = %.6g\n", s1TimeTotal / totalEmulationTime);
    printf("    average number of packets at S2 = %.6g\n", s2TimeTotal / totalEmulationTime);
    printf("\n");
    printf("    average time a packet spent in system = %.6g\n", (double) totalPacketTime / 1000000 / (double) packets);
    //printf("    standard deviation for time spent in system = %.6g\n", 0);
    printf("    standard deviation for time spent in system = 0\n");
    printf("\n");
    //printf("token drop probability = %.6g\n", 0);
    printf("token drop probability = 0\n");
    //printf("packet drop probability = %.6g\n", 0);
    printf("packet drop probability = 0\n");
    /*

    

    */
}

Packet *NewPacket(int tokensNeeded, double serviceTime, long int createdSec, long int createdMs, long int ogCreatedSec, long int ogCreatedUsec, int id) {
    Packet *packet = (Packet*)malloc(sizeof(Packet));
    packet -> tokensNeeded = tokensNeeded;
    packet -> serviceTime = serviceTime;
    packet -> createdSec = createdSec;
    packet -> createdMs = createdMs;
    packet -> ogCreatedSec = ogCreatedSec;
    packet -> ogCreatedUsec = ogCreatedUsec;
    packet -> id = id;
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

    //packet creation time
    double packet_creation_time_combined = packet_creation_time.tv_sec * 1000 + (double) packet_creation_time.tv_usec / 1000;
    long int packet_creation_time_long_int = (long int) packet_creation_time_combined;
    double packet_creation_time_left_over = packet_creation_time_combined - packet_creation_time_long_int;
    long int packet_creation_time_left_over_long_int = packet_creation_time_left_over * 1000;

    packets++;
    Packet *packet = NewPacket(
        P, 
        (double) 1/mu * 1000, 
        time_difference_long_int, 
        time_difference_left_over_long_int,
        packet_creation_time_long_int,
        packet_creation_time_left_over_long_int,
        packets
    );
    
    pthread_mutex_lock(&mutex);

    printf(
        "%08ld.%03ldms: p%d arrives, needs %d tokens, inter-arrival time: %ld.%03ldms\n", 
        time_difference_long_int, 
        time_difference_left_over_long_int, 
        packet -> id, 
        tokens,
        time_difference_long_int, 
        time_difference_left_over_long_int
    );
    interArrivalTimeTotal += time_difference_combined;

    if (tokens < P) {
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
    while (tokens_total != P) {
        usleep(1/r * 1000000);
        
        //find out when the token was generated
        //add 1/r to it
        //that's how much you need to sleep

        //end transmitting tokens when all packets have been processed
        //if ()
        
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
                    "%08ld.%03ldms: p%d leaves Q1, time in Q1 = %ld.%03ldms, token bucket now has %d token\n", 
                    time_difference_long_int, 
                    time_difference_left_over_long_int, 
                    packet -> id,
                    time_difference_packet_long_int,
                    time_difference_packet_left_over_long_int,
                    tokens
                );

                q1TimeTotal += (double) (time_difference_packet_long_int + (double) time_difference_packet_left_over_long_int / 1000);

                My402ListAppend(&q2, packet);

                TimePassed t_enter = GetTimePassed();

                //00002129.187ms: p1 enters Q2
                printf(
                    "%08ld.%03ldms: p%d enters Q2\n", 
                    t_enter.timeSec, 
                    t_enter.timeUsec, 
                    packet -> id
                );
                packet -> q2EnteredSec = t_enter.timeSec;
                packet -> q2EnteredUsec = t_enter.timeUsec;

                //wake up server threads
                pthread_cond_broadcast(&cv);



            }
        }
        pthread_mutex_unlock(&mutex);

    }
    return 0;
    
}

int TimeToQuit() {
    if (numArrived == numToArrive) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void *ServerThread(void *arg) {
    for (;;) {
        pthread_mutex_lock(&mutex);
        //time_to_quit
        
        while (My402ListLength(&q2) == 0 && numArrived != numToArrive) {
            pthread_cond_wait(&cv, &mutex);
        }

        if (numArrived == numToArrive && My402ListEmpty(&q1) && My402ListEmpty(&q2)) {
            //must unlock so the other thread can continue
            pthread_mutex_unlock(&mutex);
            pthread_cond_broadcast(&cv);
            return(0);
        } else {
            Packet *packet = (Packet*)malloc(sizeof(Packet));
            My402ListElem *elem = My402ListFirst(&q2);
            packet = elem -> obj;

            My402ListUnlink(&q2, elem);

            TimePassed t = GetTimePassed();
            //00002129.187ms: p1 enters Q2

            //format packet's creation time
            long int time_difference_packet_long_int = t.timeSec - packet -> q2EnteredSec;
            long int time_difference_packet_left_over_long_int = t.timeUsec - packet -> q2EnteredUsec;
            if (time_difference_packet_left_over_long_int < 0) {
                time_difference_packet_long_int--;
                time_difference_packet_left_over_long_int = 1000 + time_difference_packet_left_over_long_int;
            }

            printf(
                "%08ld.%03ldms: p%d leaves Q2, time in Q2 = %ld.%03ldms\n", 
                t.timeSec, 
                t.timeUsec, 
                packet -> id,
                time_difference_packet_long_int,
                time_difference_packet_left_over_long_int
            );
            q2TimeTotal += ((double) (time_difference_packet_long_int + (double) time_difference_packet_left_over_long_int / 1000));

            struct timeval packet_time, time_difference;
            gettimeofday(&packet_time, NULL);
            timersub(&packet_time, &current_time, &time_difference);

            //format time
            double time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
            long int time_difference_long_int = (long int) time_difference_combined;
            double time_difference_left_over = time_difference_combined - time_difference_long_int;
            long int time_difference_left_over_long_int = time_difference_left_over * 1000;

            printf(
                "%08ld.%03ldms: p%d begins service at S%d, requesting %04ldms of service\n", 
                time_difference_long_int, 
                time_difference_left_over_long_int, 
                packet -> id,
                arg,
                (long int) packet->serviceTime //double
            );
            
            
            //usleep(packet -> serviceTime);

            numArrived++;

            //TimePassed t2 = GetTimePassed();

            usleep(packet->serviceTime * 1000);

            //packet start time - departure time
            struct timeval packet_departure_time, packet_service_time;
            gettimeofday(&packet_departure_time, NULL);
            timersub(&packet_departure_time, &packet_time, &packet_service_time);

            //packet departure time
            struct timeval total_time_difference;
            timersub(&packet_departure_time, &current_time, &total_time_difference);

            double packet_service_time_combined = packet_service_time.tv_sec * 1000 + (double) packet_service_time.tv_usec / 1000;
            long int packet_service_time_long_int = (long int) packet_service_time_combined;
            double packet_service_time_left_over = packet_service_time_combined - packet_service_time_long_int;
            long int packet_service_time_left_over_long_int = packet_service_time_left_over * 1000;

            //time in system
            double packet_departure_time_combined = packet_departure_time.tv_sec * 1000 + (double) packet_departure_time.tv_usec / 1000;
            long int packet_departure_time_long_int = (long int) packet_departure_time_combined;
            double packet_departure_time_left_over = packet_departure_time_combined - packet_departure_time_long_int;
            long int packet_departure_time_left_over_long_int = packet_departure_time_left_over * 1000;

            double total_time_difference_combined = total_time_difference.tv_sec * 1000 + (double) total_time_difference.tv_usec / 1000;
            long int total_time_difference_long_int = (long int) total_time_difference_combined;
            double total_time_difference_left_over = total_time_difference_combined - total_time_difference_long_int;
            long int total_time_difference_left_over_long_int = total_time_difference_left_over * 1000;

            long int system_time_long_int = packet_departure_time_long_int - packet -> ogCreatedSec;
            long int system_time_left_over_long_int = packet_departure_time_left_over_long_int - packet -> ogCreatedUsec;
            if (system_time_left_over_long_int < 0) {
                system_time_long_int--;
                system_time_left_over_long_int = 1000 + system_time_left_over_long_int;
            }


            totalPacketTime += (double) system_time_long_int * 1000 + (double) system_time_left_over_long_int / 1000;
            printf("total packet time in server: %f\n", totalPacketTime);

            printf(
                "%08ld.%03ldms: p%d departs from S%d, service time = %ld.%03ldms, time in system = %ld.%03ldms\n", 
                total_time_difference_long_int, 
                total_time_difference_left_over_long_int, 
                packet -> id,
                arg,
                packet_service_time_long_int,
                packet_service_time_left_over_long_int,
                system_time_long_int,
                system_time_left_over_long_int
            );
            //total service time
            serviceTimeTotal += packet_service_time_combined;
            //service time by thread
            if (arg == 1) {
                s1TimeTotal += (double) packet_service_time_combined;
                printf("s1 time total in server thread: %f\n", s1TimeTotal);
            } else {
                s2TimeTotal += (double) packet_service_time_combined;
            }
            

            pthread_mutex_unlock(&mutex);
        }
    }
    return (0);

}
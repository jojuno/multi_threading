/*
 * Author:      Moonsoo Jo (moonsooj@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
//must include file after putting it in 'make'
#include <pthread.h>
#include <math.h>

#include "cs402.h"

#include "my402list.h"

//#define SIGINT 2

typedef struct packet {
    int tokensNeeded;
    double serviceTime;
    int interArrivalTime;
    long int q2EnteredSec;
    long int q2EnteredUsec;
    struct timeval creationTime;
    struct timeval q1Entered;
    struct timeval serviceStartTime;
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

int n = 20;
int number = 0;
char* token_bucket[100];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
My402List q1;
My402List q2;

int numToArrive = 3;
double lambda = 1;
//int interArrivalTime = 0; //ms
int serviceTime = 0; //ms
double mu = 0.35;
double r = 1.5;
int B = 10;
int P = 3;
int tokensNeeded = 0; //num tokens required
int tokensInBucket = 0;
int tokensTotal = 0;
int packetsSentToServers = 0;
int packetsProcessedByServers = 0;
int tokensLost = 0;
int packetsSentFromQ1 = 0;
int packetsSentToQ2 = 0;
double avgSystemDuration = 0;
double avgSystemDurationSquared = 0;
int packetsLost = 0;
//packets total, packets that passed, packets dropped

struct timeval emulation_start_time;
int packetsTotal = 0;
int numPacketsExpected = 0;
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
struct timeval lastPacketArrivedTime;
FILE *fp;
int traceDrivenMode = FALSE;
sigset_t set;

int sigIntPressed = 0;



void *PacketThread(void *);
void *TokenThread(void *);
void *ServerThread(void *);
void *SigIntThread(void *);

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
    timersub(&creation_time, &emulation_start_time, &time_difference);

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

    TimePassed t;
    t.timeSec = time_difference_long_int;
    t.timeUsec = time_difference_left_over_long_int;

    return t;

}

int main(int argc, char *argv[])
{
    char bucketArg[3] = "-B";
    char textFileArg[3] = "-t";
    char rateOfTokensArg[3] = "-r";
    char numPacketsArg[3] = "-n";
    char numTokensPerPacketArg[3] = "-P";
    char lambdaArg[8] = "-lambda";
    char muArg[4] = "-mu";
    char textFileName[50];
    char *rateOfTokensPtr;
    char *lambdaPtr;
    char *muPtr;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], textFileArg) == 0) {
            if (argv[i+1][0] != '-') {
                strncpy(textFileName, argv[i+1], sizeof(textFileName));
                fp = fopen(textFileName, "r");
                traceDrivenMode = TRUE;
                continue;
            }
        }
        if (strcmp(argv[i], bucketArg) == 0) {
            if (argv[i+1][0] != '-') {
                B = atoi(argv[i+1]);
                continue;
            }
        }
        if (strcmp(argv[i], rateOfTokensArg) == 0) {
            if (argv[i+1][0] != '-') {
                r = strtod(argv[i+1], &rateOfTokensPtr);
                continue;
            }
        }
        if (strcmp(argv[i], numPacketsArg) == 0) {
            if (argv[i+1][0] != '-') {
                n = atoi(argv[i+1]);
                continue;
            }
        }
        
        if (strcmp(argv[i], numTokensPerPacketArg) == 0) {
            if (argv[i+1][0] != '-') {
                P = atoi(argv[i+1]);
                continue;
            }
        }
        if (strcmp(argv[i], lambdaArg) == 0) {
            if (argv[i+1][0] != '-') {
                lambda = strtod(argv[i+1], &lambdaPtr);
                continue;
            }
        }
        if (strcmp(argv[i], muArg) == 0) {
            if (argv[i+1][0] != '-') {
                mu = strtod(argv[i+1], &muPtr);
                continue;
            }
        }
        
    }

    if (traceDrivenMode) {
        char buf[2000];
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            //first line
            sscanf(buf, "%d", &numPacketsExpected);
            n = numPacketsExpected;
        }
    }


    //success
    memset(&q1, 0, sizeof(q1));
    if (!My402ListInit(&q1)) {
        fprintf(stderr, "list not initialized\n");
    }

    //time of start
    gettimeofday(&emulation_start_time, NULL);
    //current_time.tv_sec, current_time.tv_usec
    printf("00000000.000ms: emulation begins\n");

    //threads
    pthread_t packetThread;
    pthread_t tokenThread;
    pthread_t serverThread1;
    pthread_t serverThread2;
    pthread_t sigIntThread;

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
    
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, 0);
    pthread_create(&sigIntThread, 0, SigIntThread, 0);

    pthread_join(packetThread, 0);
    pthread_join(tokenThread, 0);
    pthread_join(serverThread1, 0);
    pthread_join(serverThread2, 0);

    //token arrival time
    struct timeval emulation_finished_time, emulation_finished_time_difference;
    gettimeofday(&emulation_finished_time, NULL);

    TimePassed emulation_finished_time_passed = GetTimePassed2(emulation_finished_time, emulation_start_time);
    printf(
        "%08ld.%03ldms: emulation ends\n", 
        emulation_finished_time_passed.timeSec, 
        emulation_finished_time_passed.timeUsec
    );
    timersub(&emulation_finished_time, &emulation_start_time, &emulation_finished_time_difference);
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
    printf("    average packet inter-arrival time = %.5f\n", interArrivalTimeTotal / (packetsTotal) / 1000);
    printf("    average packet service time = %.5f\n", serviceTimeTotal / (packetsTotal - packetsLost) / 1000);
    printf("\n");
    //divide number into whole number and decimal number
    printf("    average number of packets in Q1 = %.5f\n", q1TimeTotal / (double) totalEmulationTime);
    printf("    average number of packets in Q2 = %.6g\n", (double) q2TimeTotal / (double) totalEmulationTime);
    printf("    average number of packets at S1 = %.6g\n", s1TimeTotal / totalEmulationTime);
    printf("    average number of packets at S2 = %.6g\n", s2TimeTotal / totalEmulationTime);
    printf("\n");
    printf("    average time a packet spent in system = %.6g\n", (double) totalPacketTime / 1000 / (double) (packetsTotal - packetsLost));
    //printf("    average system duration squared: %.6g\n", avgSystemDurationSquared);
    //printf("    average system duration: %.6g\n", avgSystemDuration);
    printf("    standard deviation for time spent in system = %.6g\n", sqrt((double) (avgSystemDurationSquared) - (double) (avgSystemDuration) * (double) (avgSystemDuration)));
    printf("\n");
    printf("    token drop probability = %.6g\n", (double) ((double) tokensLost / (double) tokensTotal));
    printf("    packet drop probability = %.6g\n", (double) ((double) packetsLost / (double) packetsTotal));
}

Packet *NewPacket(int tokensNeeded, double serviceTime, int interArrivalTime, int id, struct timeval enteredTime) {
    Packet *packet = (Packet*)malloc(sizeof(Packet));
    packet -> tokensNeeded = tokensNeeded;
    packet -> serviceTime = serviceTime;
    packet -> interArrivalTime = interArrivalTime;
    packet -> id = id;
    packet -> creationTime = enteredTime;
    return packet;
}

void *SigIntThread(void *arg) {
    int sig;
    printf("started sig int thread\n");
    for (;;) {
        printf("sig waiting\n");
        sigwait(&set, &sig);
        printf("sig wait over\n");
        pthread_mutex_lock(&mutex);
        printf("SIGINT caught, no new packets or tokens will be allowed\n");
        //pthread_mutex_unlock(&mutex);
        //fflush(stdout);
    }
}

void *PacketThread(void *arg) {

    for (;;) {
        if (packetsSentFromQ1 == n) {
            return 0;
        }
        int interArrivalTime;
        //int tokens;
        //int serviceTime;
        if (traceDrivenMode) {
            char buf[2000];
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                //inter arrival time, number of tokens required, service time
                if (sscanf(buf, "%d %d %d", &interArrivalTime, &tokensNeeded, &serviceTime) == 3) {
                    serviceTime = (double) serviceTime;
                    interArrivalTime *= 1000;
                } else {
                    fprintf(stderr, "input not in the right format.\n");
                    exit(-1);
                }
            }
        } else {
            tokensNeeded = P;
            interArrivalTime = 1 / lambda * 1000000;
            serviceTime = (double) 1/mu * 1000;
        }

        usleep(interArrivalTime);
        
        pthread_mutex_lock(&mutex);

        //packet arrival time
        struct timeval packet_creation_time, inter_arrival_time_measured, total_time;
        gettimeofday(&packet_creation_time, NULL);
        if (packetsTotal == 0) {
            lastPacketArrivedTime = emulation_start_time;
        }

        timersub(&packet_creation_time, &lastPacketArrivedTime, &inter_arrival_time_measured);
        timersub(&packet_creation_time, &emulation_start_time, &total_time);

        //inter arrival time
        double inter_arrival_time_measured_combined = inter_arrival_time_measured.tv_sec * 1000 + (double) inter_arrival_time_measured.tv_usec / 1000;
        long int inter_arrival_time_measured_long_int = (long int) inter_arrival_time_measured_combined;
        double inter_arrival_time_measured_left_over = inter_arrival_time_measured_combined - inter_arrival_time_measured_long_int;
        long int inter_arrival_time_measured_left_over_long_int = inter_arrival_time_measured_left_over * 1000;

        //packet creation time
        double packet_creation_time_combined = packet_creation_time.tv_sec * 1000 + (double) packet_creation_time.tv_usec / 1000;
        long int packet_creation_time_long_int = (long int) packet_creation_time_combined;
        double packet_creation_time_left_over = packet_creation_time_combined - packet_creation_time_long_int;
        long int packet_creation_time_left_over_long_int = packet_creation_time_left_over * 1000;

        //total time
        double total_time_combined = total_time.tv_sec * 1000 + (double) total_time.tv_usec / 1000;
        long int total_time_long_int = (long int) total_time_combined;
        double total_time_left_over = total_time_combined - total_time_long_int;
        long int total_time_left_over_long_int = total_time_left_over * 1000;

        //if ctrl+c is pressed, do not generate the packet
        /*
        *
        * 
        * 
        * 
        packet generation
        *
        * 
        * 
        * 
         */
        packetsTotal++;
        Packet *packet = NewPacket(
            tokensNeeded, 
            serviceTime, 
            interArrivalTime,
            packetsTotal,
            packet_creation_time
        );

        lastPacketArrivedTime = packet_creation_time;
        interArrivalTimeTotal += inter_arrival_time_measured_combined;
        
        if (tokensNeeded > B) {
            printf(
                "%08ld.%03ldms: p%d arrives, needs %d tokens, inter-arrival time: %ld.%03ldms, dropped\n", 
                total_time_long_int, 
                total_time_left_over_long_int, 
                packet -> id, 
                tokensNeeded,
                inter_arrival_time_measured_long_int, 
                inter_arrival_time_measured_left_over_long_int
            );
            packetsLost++;
            packetsSentFromQ1++;
            //update number to set the conditionals
            pthread_mutex_unlock(&mutex);
            continue;
        } else if (tokensNeeded > 1) {
            
            printf(
                "%08ld.%03ldms: p%d arrives, needs %d tokens, inter-arrival time: %ld.%03ldms\n", 
                total_time_long_int, 
                total_time_left_over_long_int, 
                packet -> id, 
                tokensNeeded,
                inter_arrival_time_measured_long_int, 
                inter_arrival_time_measured_left_over_long_int
            );
        } else {
            printf(
                "%08ld.%03ldms: p%d arrives, needs %d token, inter-arrival time: %ld.%03ldms\n", 
                total_time_long_int, 
                total_time_left_over_long_int, 
                packet -> id, 
                tokensNeeded,
                inter_arrival_time_measured_long_int, 
                inter_arrival_time_measured_left_over_long_int
            );
        }
  
        
        
        My402ListAppend(&q1, packet);
        packetsSentFromQ1++;
        

        //packet arrival time
        struct timeval q1_entered_time;
        gettimeofday(&q1_entered_time, NULL);
        timersub(&q1_entered_time, &emulation_start_time, &total_time);
        packet -> q1Entered = q1_entered_time;

        //format time
        total_time_combined = total_time.tv_sec * 1000 + (double) total_time.tv_usec / 1000;
        total_time_long_int = (long int) total_time_combined;
        total_time_left_over = total_time_combined - total_time_long_int;
        total_time_left_over_long_int = total_time_left_over * 1000;

        printf(
            "%08ld.%03ldms: p%d enters Q1\n", 
            total_time_long_int, 
            total_time_left_over_long_int, 
            packet -> id
        );

        My402ListElem *first_elem = My402ListFirst(&q1);
        Packet *first_packet = first_elem -> obj;
        if (tokensInBucket >= first_packet -> tokensNeeded) {
            tokensInBucket -= first_packet -> tokensNeeded;
            My402ListElem *elem = My402ListFirst(&q1);
            Packet *packet_first = elem -> obj;
            My402ListElem *new_elem = (My402ListElem*)malloc(sizeof(My402ListElem));
            new_elem -> obj = &packet_first;

            struct timeval q1_departure_time, q1_duration, total_time;
            gettimeofday(&q1_departure_time, NULL);
            timersub(&q1_departure_time, &emulation_start_time, &total_time);
            timersub(&q1_departure_time, &(packet -> q1Entered), &q1_duration);

            double total_time_combined = total_time.tv_sec * 1000 + (double) total_time.tv_usec / 1000;
            long int total_time_long_int = (long int)total_time_combined;
            double total_time_left_over = total_time_combined - total_time_long_int;
            long int total_time_left_over_long_int = total_time_left_over * 1000;

            //format time
            double q1_duration_combined = q1_duration.tv_sec * 1000 + (double) q1_duration.tv_usec / 1000;
            long int q1_duration_long_int = (long int) q1_duration_combined;
            double q1_duration_left_over = q1_duration_combined - q1_duration_long_int;
            long int q1_duration_left_over_long_int = q1_duration_left_over * 1000;

            if (tokensInBucket > 1) {
                printf(
                    "%08ld.%03ldms: p%d leaves Q1, time in Q1 = %ld.%03ldms, token bucket now has %d tokens\n", 
                    total_time_long_int, 
                    total_time_left_over_long_int, 
                    packet -> id,
                    q1_duration_long_int,
                    q1_duration_left_over_long_int,
                    tokensInBucket
                );
            } else {
                printf(
                    "%08ld.%03ldms: p%d leaves Q1, time in Q1 = %ld.%03ldms, token bucket now has %d token\n", 
                    total_time_long_int, 
                    total_time_left_over_long_int, 
                    packet -> id,
                    q1_duration_long_int,
                    q1_duration_left_over_long_int,
                    tokensInBucket
                );
            }


            q1TimeTotal += (double) (q1_duration_long_int + (double) q1_duration_left_over_long_int / 1000);

            My402ListAppend(&q2, packet);

            TimePassed t_enter = GetTimePassed();

            printf(
                "%08ld.%03ldms: p%d enters Q2\n", 
                t_enter.timeSec, 
                t_enter.timeUsec, 
                packet -> id
            );
            packet -> q2EnteredSec = t_enter.timeSec;
            packet -> q2EnteredUsec = t_enter.timeUsec;

            My402ListUnlink(&q1, My402ListFirst(&q1));
            packetsSentToQ2++;
            pthread_cond_broadcast(&cv);

            if (packetsSentToQ2 == (n - packetsLost)) {
                pthread_mutex_unlock(&mutex);
                return 0;
            }
            
        }

        pthread_mutex_unlock(&mutex);
    }

    return (0);

}


//generates tokens at a steady rate
//when enough token is generated
//transmit the packet into the server
void *TokenThread(void *arg) {
    //infinite loop
    for (;;) {
        if (packetsSentToQ2 == (n - packetsLost)) {
            pthread_mutex_unlock(&mutex);
            pthread_cond_broadcast(&cv);
            return 0;
        }
        //keep generating for now
        usleep(1/r * 1000000);
        
        //find out when the token was generated
        //add 1/r to it
        //that's how much you need to sleep

        //end transmitting tokens when all packets have been processed
        //if ()
        
        pthread_mutex_lock(&mutex);

        //if ctrl+c is pressed, do not generate the token
        /*
        *
        * 
        * 
        * 
        token generation
        *
        * 
        * 
        * 
         */
        tokensTotal++;

        //token arrival time
        struct timeval token_creation_time, time_difference;
        gettimeofday(&token_creation_time, NULL);
        timersub(&token_creation_time, &emulation_start_time, &time_difference);

        double time_difference_combined = time_difference.tv_sec * 1000 + (double) time_difference.tv_usec / 1000;
        long int time_difference_long_int = (long int) time_difference_combined;
        double time_difference_left_over = time_difference_combined - time_difference_long_int;
        long int time_difference_left_over_long_int = time_difference_left_over * 1000;

        //for the amount in the bucket
        if (tokensInBucket == B) {
            printf(
                "%08ld.%03ldms: token t%d arrives, dropped\n", 
                time_difference_long_int, 
                time_difference_left_over_long_int, 
                tokensTotal, 
                tokensInBucket
            );
            tokensLost++;
        } else {
            tokensInBucket++;

            //pluralize "token"
            if (tokensInBucket > 1) {
                printf(
                    "%08ld.%03ldms: token t%d arrives, token bucket now has %d tokens\n", 
                    time_difference_long_int, 
                    time_difference_left_over_long_int, 
                    tokensTotal, 
                    tokensInBucket
                );
            } else {
                printf(
                    "%08ld.%03ldms: token t%d arrives, token bucket now has %d token\n", 
                    time_difference_long_int, 
                    time_difference_left_over_long_int, 
                    tokensTotal, 
                    tokensInBucket
                );
            }

            //check if there's enough tokens to remove a packet
            if (My402ListEmpty(&q1) != TRUE && packetsSentToQ2 != (n - packetsLost)) {
                My402ListElem *elem = My402ListFirst(&q1);
                Packet *packet_first = elem -> obj;
                if (tokensInBucket >= packet_first -> tokensNeeded) {
                    My402ListElem *new_elem = (My402ListElem*)malloc(sizeof(My402ListElem));
                    new_elem -> obj = &packet_first;
                    tokensInBucket -= packet_first -> tokensNeeded;

                    struct timeval q1_departure_time, q1_duration, total_time;
                    gettimeofday(&q1_departure_time, NULL);
                    timersub(&q1_departure_time, &emulation_start_time, &total_time);
                    timersub(&q1_departure_time, &(packet_first -> q1Entered), &q1_duration);

                    double total_time_combined = total_time.tv_sec * 1000 + (double) total_time.tv_usec / 1000;
                    long int total_time_long_int = (long int)total_time_combined;
                    double total_time_left_over = total_time_combined - total_time_long_int;
                    long int total_time_left_over_long_int = total_time_left_over * 1000;

                    //format time
                    double q1_duration_combined = q1_duration.tv_sec * 1000 + (double) q1_duration.tv_usec / 1000;
                    long int q1_duration_long_int = (long int) q1_duration_combined;
                    double q1_duration_left_over = q1_duration_combined - q1_duration_long_int;
                    long int q1_duration_left_over_long_int = q1_duration_left_over * 1000;

                    if (tokensInBucket > 1) {
                        printf(
                            "%08ld.%03ldms: p%d leaves Q1, time in Q1 = %ld.%03ldms, token bucket now has %d tokens\n", 
                            total_time_long_int, 
                            total_time_left_over_long_int, 
                            packet_first -> id,
                            q1_duration_long_int,
                            q1_duration_left_over_long_int,
                            tokensInBucket
                        );
                    } else {
                        printf(
                            "%08ld.%03ldms: p%d leaves Q1, time in Q1 = %ld.%03ldms, token bucket now has %d token\n", 
                            total_time_long_int, 
                            total_time_left_over_long_int, 
                            packet_first -> id,
                            q1_duration_long_int,
                            q1_duration_left_over_long_int,
                            tokensInBucket
                        );
                    }


                    q1TimeTotal += (double) (q1_duration_long_int + (double) q1_duration_left_over_long_int / 1000);

                    My402ListAppend(&q2, packet_first);

                    TimePassed t_enter = GetTimePassed();

                    printf(
                        "%08ld.%03ldms: p%d enters Q2\n", 
                        t_enter.timeSec, 
                        t_enter.timeUsec, 
                        packet_first -> id
                    );
                    packet_first -> q2EnteredSec = t_enter.timeSec;
                    packet_first -> q2EnteredUsec = t_enter.timeUsec;

                    My402ListUnlink(&q1, My402ListFirst(&q1));
                    packetsSentToQ2++;

                    //wake up server threads
                    pthread_cond_broadcast(&cv);
                    if (packetsSentToQ2 == (n - packetsLost)) {
                        
                        pthread_mutex_unlock(&mutex);
                        return 0;
                    }
                }
            } else if (My402ListEmpty(&q2) && packetsSentToQ2 == (n - packetsLost)) {
                pthread_mutex_unlock(&mutex);
                return 0;
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
        
        while (My402ListLength(&q2) == 0 && packetsSentToQ2 != (n - packetsLost)) {
            pthread_mutex_unlock(&mutex);
            pthread_cond_wait(&cv, &mutex);
        }

        if (packetsSentToQ2 == (n - packetsLost) && My402ListEmpty(&q1) && My402ListEmpty(&q2)) {
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
            pthread_mutex_unlock(&mutex);
            numArrived++;

            struct timeval serviceStartTime, time_difference;
            gettimeofday(&serviceStartTime, NULL);
            timersub(&serviceStartTime, &emulation_start_time, &time_difference);

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

            packet -> serviceStartTime = serviceStartTime;
            

            usleep(packet->serviceTime * 1000);

            //pthread_mutex_lock(&mutex);
            

            //packet departure time
            struct timeval packet_departure_time;
            gettimeofday(&packet_departure_time, NULL);

            //total time
            //service duration
            //system duration

            //total time (since the beginning)
            struct timeval total_time;
            timersub(&packet_departure_time, &emulation_start_time, &total_time);
            double total_time_combined = total_time.tv_sec * 1000 + (double) total_time.tv_usec / 1000;
            long int total_time_long_int = (long int) total_time_combined;
            double total_time_left_over = total_time_combined - total_time_long_int;
            long int total_time_left_over_long_int = total_time_left_over * 1000;

            //service duration
            struct timeval packet_service_duration;
            timersub(&packet_departure_time, &(packet -> serviceStartTime), &packet_service_duration);
            double packet_service_duration_combined = packet_service_duration.tv_sec * 1000 + (double) packet_service_duration.tv_usec / 1000;
            long int packet_service_duration_long_int = (long int) packet_service_duration_combined;
            double packet_service_duration_left_over = packet_service_duration_combined - packet_service_duration_long_int;
            long int packet_service_duration_left_over_long_int = packet_service_duration_left_over * 1000;

            //system duration
            struct timeval system_duration;
            timersub(&packet_departure_time, &(packet -> creationTime), &system_duration);
            double system_duration_combined = system_duration.tv_sec * 1000 + (double) system_duration.tv_usec / 1000;
            long int system_duration_long_int = (long int) system_duration_combined;
            double system_duration_left_over = system_duration_combined - system_duration_long_int;
            long int system_duration_left_over_long_int = system_duration_left_over * 1000;


            //if ctrl+c is pressed, do not transmit the packet
            /*
            *
            * 
            * 
            * 
            server processing packet
            *
            * 
            * 
            * 
            */

            totalPacketTime += system_duration_combined;

            printf(
                "%08ld.%03ldms: p%d departs from S%d, service time = %ld.%03ldms, time in system = %ld.%03ldms\n", 
                total_time_long_int, 
                total_time_left_over_long_int, 
                packet -> id,
                arg,
                packet_service_duration_long_int,
                packet_service_duration_left_over_long_int,
                system_duration_long_int,
                system_duration_left_over_long_int
            );
            //total service time
            serviceTimeTotal += packet_service_duration_combined;
            

            //running average of system duration
            avgSystemDuration = ((packetsProcessedByServers * avgSystemDuration + system_duration_combined / 1000)) / (packetsProcessedByServers + 1);
            //running average of system duration squared
            avgSystemDurationSquared = ((packetsProcessedByServers * avgSystemDurationSquared + ((system_duration_combined / 1000) * (system_duration_combined / 1000))) / (packetsProcessedByServers + 1));
            packetsProcessedByServers++;

            //service time by thread
            if (arg == 1) {
                s1TimeTotal += (double) packet_service_duration_combined;
            } else {
                s2TimeTotal += (double) packet_service_duration_combined;
            }
            
            //pthread_mutex_unlock(&mutex);
        }
    }
    return (0);

}
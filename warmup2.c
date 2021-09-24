/*
 * Author:      Moonsoo Jo (moonsooj@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>



static
void Usage()
{
    fprintf(stderr,
            "usage: %s\n",
            "./warmup2 ");
    exit(-1);
}

int number = 0;

void *PacketThread(void *);
void *TokenThread(void *);
void *ServerThread(void *);

/* ----------------------- main() ----------------------- */

int main(int argc, char *argv[])
{
    
    pthread_t packetThread;
    pthread_t tokenThread;
    pthread_t serverThread1;
    pthread_t serverThread2;
    int arg1 = 1;
    int arg2 = 2;
    int arg3 = 3;
    int argument = 1;
    int result;

    pthread_create(&packetThread, 0, PacketThread, (void *)arg1);
    pthread_create(&tokenThread, 0, TokenThread, (void *)argument);
    pthread_create(&serverThread1, 0, PacketThread, (void *)arg2);
    pthread_create(&serverThread2, 0, PacketThread, (void *)arg3);

    pthread_join(packetThread, &result);
    printf("result of packetThread: %d\n", result);
    pthread_join(tokenThread, 0);
    pthread_join(serverThread1, 0);
    pthread_join(serverThread2, 0);

    printf("number after operations: %d\n", number);

    pthread_exit(0);

    return(0);
}

void *PacketThread(void *arg) {
    if ((int)arg == 3) {
        printf("thread: server 2\n");
    } else if ((int)arg == 2) {
        printf("thread: server 1\n");
    } else if ((int)arg == 1) {
        printf("thread: packet\n");
    }
    

    number += 1;
    pthread_exit((void *)1);
    //return (0);
}

void *TokenThread(void *arg) {
    number += 1;
    return (0);
}
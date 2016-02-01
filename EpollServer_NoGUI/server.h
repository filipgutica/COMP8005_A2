#ifndef SERVER_H
#define SERVER_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <iostream>

#define TRUE 		1
#define FALSE 		0
#define EPOLL_QUEUE_LEN	256
#define BUFLEN		1024
#define SERVER_PORT	7000

typedef struct thrdParams
{
    int fd;
    int thrdResult;
} thrdParams;


void SystemFatal (const char* message);
void* ClearSocket (void *param);
void* HandleClient(void *param);
void close_fd (int);
void* UpdateConsole(void *param);


#endif // SERVER_H

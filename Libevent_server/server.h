#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* For inet_ntoa. */
#include <arpa/inet.h>

/* Required by event.h. */
#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "queue.h"

/* Libevent. */
#include <event.h>

/* Port to listen on. */
#define SERVER_PORT 5555

/* Length of each buffer in the buffer queue.  Also becomes the amount
 * of data we try to read per call to read(2). */
#define BUFLEN 1024

class server
{
public:
    server();

private:
    struct bufferq {
        /* The buffer. */
        u_char *buf;

        /* The length of buf. */
        int len;

        /* The offset into buf to start writing from. */
        int offset;

        /* For the linked list structure. */
        TAILQ_ENTRY(bufferq) entries;
    };

    struct client {
        /* Events. We need 2 event structures, one for read event
         * notification and the other for writing. */
        struct event ev_read;
        struct event ev_write;

        /* This is the queue of data to be written to this client. As
         * we can't call write(2) until libevent tells us the socket
         * is ready for writing. */
        TAILQ_HEAD(, bufferq) writeq;
    };

    static int setnonblock(int fd);
    static void on_read(int fd, short ev, void *arg);
    static void on_write(int fd, short ev, void *arg);
    static void  on_accept(int fd, short ev, void *arg);
};

#endif // SERVER_H

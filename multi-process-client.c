/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		epoll_clnt.c - A simple TCP client program.
--
--	PROGRAM:		epollc
--				gcc -Wall -ggdb -o epollc epoll_clnt.c
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			February 2, 2008
--
--	REVISIONS:		(Date and Description)
--				January 2005
--				Modified the read loop to use fgets.
--				While loop is based on the buffer length
--
--
--	DESIGNERS:		Aman Abdulla
--
--	PROGRAMMERS:		Aman Abdulla
--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
-- 	The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user will be
-- 	prompted for date. The date string is then sent to the server and the
-- 	response (echo) back from the server is displayed.
--	This client application can be used to test the aaccompanying epoll
--	server: epoll_svr.c
---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_TCP_PORT		7000	// Default port
#define BUFLEN			1024  	// Buffer length
#define NUM_CLIENTS 10000
#define NUM_MESSAGES 10

int main (int argc, char **argv)
{
	int n, bytes_to_read, i;
	int sd, port;
	struct hostent	*hp;
	struct sockaddr_in server;
	char  *host;
	char str[16];

	switch(argc)
	{
		case 2:
			host =	argv[1];	// Host name
			port =	SERVER_TCP_PORT;
		break;
		case 3:
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
		break;
		default:
			fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
			exit(1);
	}

 for (i = 0; i < NUM_CLIENTS; i++)
 {
		// usleep(10000);
	 if (fork() == 0)
	 {

		 clock_t start = clock(), diff;

		  char *bp, rbuf[BUFLEN], sbuf[BUFLEN], **pptr;
			memset(sbuf, 0, BUFLEN);
			// Create the socket
			if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				perror("Cannot create socket");
				exit(1);
			}
			bzero((char *)&server, sizeof(struct sockaddr_in));
			server.sin_family = AF_INET;
			server.sin_port = htons(port);
			if ((hp = gethostbyname(host)) == NULL)
			{
				fprintf(stderr, "Unknown server address\n");
				exit(1);
			}
			bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

			// Connecting to the server
			if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
			{
				fprintf(stderr, "Can't connect to server\n");
				perror("connect");
				exit(1);
			}
			printf("Connected:    Server Name: %s\n", hp->h_name);
			pptr = hp->h_addr_list;
			printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));

			int j = 0;
			for (j = 0; j< NUM_MESSAGES; j++)
			{
				sleep(1);
			//	printf("Transmit:\n");

			// get user's text

				sprintf (sbuf, "Hello from client!!\n");

				// Transmit data through the socket

				//sleep(1);
				send (sd, sbuf, BUFLEN, 0);



			//	printf("Receive:\n");
				bp = rbuf;
				bytes_to_read = BUFLEN;

				// client makes repeated calls to recv until no more data is expected to arrive.
				n = 0;
				while ((n = recv (sd, bp, bytes_to_read, 0)) < BUFLEN)
				{
					bp += n;
					bytes_to_read -= n;
				}
				printf ("%s\n", rbuf);
			}
			sleep(10);

			//printf("CLOSING SOCKET\n");

			fflush(stdout);
			close (sd);
			exit(0);
		}
	}

	return (0);
}

#include "server.h"
#include "statics.h"
#include <time.h>       /* time */

struct epoll_event Server::events[EPOLL_QUEUE_LEN], Server::event;
struct epoll_event Server::worker_events[NUM_WORKERS][EPOLL_QUEUE_LEN];
struct epoll_event Server::worker_event[NUM_WORKERS];
int numClients;
pthread_mutex_t worker_mutex;

Server::Server()
{
    srand (time(NULL));
    qDebug() << "Inside server";
    numClients = 0;
    port = SERVER_PORT;
    addr_size = sizeof(struct sockaddr_in);

    thrdInfo = new thrdParams();
    for (int i = 0; i< NUM_WORKERS; i++)
        workerParams[i] = new thrdParams();

    numClients = 0;

    QThreadPool::globalInstance()->setMaxThreadCount(100000);
    pthread_create(&updateConsoleThrd, NULL, &UpdateConsole, (void*)1);

    // Create the listening socket
    fd_server = socket (AF_INET, SOCK_STREAM, 0);
    if (fd_server == -1)
        SystemFatal("socket");

    // set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
    arg = 1;
    if (setsockopt (fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
        SystemFatal("setsockopt");

    // Make the server listening socket non-blocking
    if (fcntl (fd_server, F_SETFL, O_NONBLOCK | fcntl (fd_server, F_GETFL, 0)) == -1)
        SystemFatal("fcntl");

    // Bind to the specified listening port
    memset (&addr, 0, sizeof (struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind (fd_server, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        SystemFatal("bind");

    // Listen for fd_news; SOMAXCONN is 128 by default
    if (listen (fd_server, SOMAXCONN) == -1)
        SystemFatal("listen");

    // Create the epoll file descriptor
    epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if (epoll_fd == -1)
        SystemFatal("epoll_create");

    // Add the server socket to the epoll Server::event loop
    Server::event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;
    Server::event.data.fd = fd_server;
    if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_server, &Server::event) == -1)
        SystemFatal("epoll_ctl");

}

void Server::startServer()
{

    // Execute the epoll Server::event loop

    pthread_t workerThrds[NUM_WORKERS];
    pthread_t acceptThrd;

    thrdInfo->epoll_fd = epoll_fd;
    thrdInfo->fd_new = fd_new;
    thrdInfo->fd_server = fd_server;

    for (int i = 0; i < NUM_WORKERS; i++)
    {
        worker_fds[i] = epoll_create(EPOLL_QUEUE_LEN);
        if (worker_fds[i] == -1)
            SystemFatal("epoll_create");

        thrdInfo->worker_fds[i] = worker_fds[i];
        workerParams[i]->worker_fds[i] = worker_fds[i];
        Server::worker_event[i].events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;
    }

    pthread_create(&acceptThrd, NULL, &acceptConnections, (void*)thrdInfo);


    for (int i = 0; i< NUM_WORKERS; i++)
    {
      workerParams[i]->thrdNumber = i;
      pthread_create(&workerThrds[i], NULL, &worker, (void*)workerParams[i]);
    }

    for (int i = 0; i< NUM_WORKERS; i++)
    {
      pthread_join(workerThrds[i], NULL);
      std::cout << "Thread created";
    }

    close(fd_server);
    delete thrdInfo;
    delete workerParams;
    return;
}

void *acceptConnections(void *param)
{
    thrdParams *thrdInfo = (thrdParams*)param;
    struct sockaddr_in remote_addr;

    while(TRUE)
    {
        int num_fds = epoll_wait (thrdInfo->epoll_fd, Server::events, EPOLL_QUEUE_LEN, -1);

        if (num_fds < 0)
        {
            std::cout << "Error in epoll_wait!" ;
            exit(-1);
        }

        for (int i = 0; i < num_fds; i++)
        {
            // Case 1: Error condition
            if (Server::events[i].events & (EPOLLHUP | EPOLLERR))
            {
                numClients--;
                std::cout << "EPOLL ERROR" << std::endl;
                close(Server::events[i].data.fd);
                continue;
            }


            // Case 2: Server is receiving a connection request
            if (Server::events[i].data.fd == thrdInfo->fd_server)
            {
                socklen_t addr_size = sizeof(remote_addr);
                thrdInfo->fd_new = accept (thrdInfo->fd_server, (struct sockaddr*) &remote_addr, &addr_size);
                if (thrdInfo->fd_new == -1)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        perror("accept");
                    }
                    continue;
                }

                // Make the fd_new non-blocking
                if (fcntl (thrdInfo->fd_new, F_SETFL, O_NONBLOCK | fcntl(thrdInfo->fd_new, F_GETFL, 0)) == -1)
                {
                    std::cout << "fcntl";
                    exit(-1);
                }

                // Add the new socket descriptor to the epoll loop
                int n = rand() % 4;
                cout << "Assigning to thread: " << n << std::endl;
                Server::worker_event[n].data.fd = thrdInfo->fd_new;
                if (epoll_ctl (thrdInfo->worker_fds[n], EPOLL_CTL_ADD, thrdInfo->fd_new, &Server::worker_event[n]) == -1)
                {
                    std::cout << "epoll_ctl";
                    exit(-1);
                }



                //std::cout << " Remote Address: " << inet_ntoa(remote_addr.sin_addr) << std::endl;
                numClients++;

                //std::cout << "Clients Connected: " << numClients << std::endl;
                continue;
            }
        }

    }
}

void* worker(void* param)
{
    thrdParams *thrdInfo = (thrdParams*)param;
    int index = thrdInfo->thrdNumber;
    std::cout<< "thread  index " << index << std::endl;
    while(TRUE)
    {
        //struct epoll_Server::event Server::events[MAX_Server::eventS];
        int num_fds = epoll_wait (thrdInfo->worker_fds[index], Server::worker_events[index], EPOLL_QUEUE_LEN, -1);

        if (num_fds < 0)
        {
            std::cout << "Error in epoll_wait!" ;
            exit(-1);
        }

        //pthread_mutex_lock(&worker_mutex);
        for (int i = 0; i < num_fds; i++)
        {

            // Case 1: Error condition
            if (Server::worker_events[index][i].events & (EPOLLHUP | EPOLLERR))
            {
                numClients--;
                std::cout << "EPOLL ERROR" << std::endl;
                close(Server::worker_events[index][i].data.fd);
                continue;
            }
            //assert (Server::worker_events[index][i].events & EPOLLIN);

            if (Server::worker_events[index][i].events & (EPOLLIN))
            {
                thrdInfo->fd = Server::worker_events[index][i].data.fd;
                readSocket((void*)thrdInfo);

               // QFuture<void> future = QtConcurrent::run(readSocket, (void*)thrdInfo);
               // future.waitForFinished();

            }

            if ( Server::worker_events[index][i].events & (EPOLLRDHUP))
            {
                close(Server::worker_events[index][i].data.fd);
                numClients--;
                //std::cout << "Clients Connected: " << numClients <<  std::endl;
            }
            //std::cout << "\r" << "Clients Connected: " << numClients <<  std::flush;

        }
        //pthread_mutex_unlock(&worker_mutex);

    }
}


void Server::SystemFatal(const char *message)
{
    std::cout << message << std::endl;
      exit (EXIT_FAILURE);
}


void Server::close_fd(int signo)
{
    close(fd_server);
        exit (EXIT_SUCCESS);
}

void* UpdateConsole(void *param)
{
    while(TRUE)
    {
        usleep(100000);
        std::cout << "Clients Connected: " << numClients <<  std::endl;
    }
}


void *readSocket(void *param)
{
    int	n, bytes_to_read;
        char	*bp, buf[BUFLEN];
        thrdParams* data = (thrdParams*) param;
        int fd = data->fd;
        int index = data->eventIndex;

        bp = buf;
        bytes_to_read = BUFLEN;
       /* while ((n = recv (fd, bp, bytes_to_read, 0)) < BUFLEN)
        {
            bp += n;
            bytes_to_read -= n;


            if (n == 0)
            {
                break;
                //qDebug() << "Client Disconnect";
                //close(fd);
                //app->ClientDisconnect();

                //pthread_exit(0);
            }
            if (n == -1)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    break;
                  //  pthread_exit(0);
                }
                continue;
            }

        }*/
        //std::cout << "Sending: " << buf << std::endl;
        n = recv (fd, bp, bytes_to_read, 0);

        if (n == 0)
        {
            return NULL;
        }
        if (n == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                return NULL;
            }
        }

        send (fd, buf, BUFLEN, 0);
               // pthread_exit(0); // Return n bytes read immediately we dont want this thread running forever.

}



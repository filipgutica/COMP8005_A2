#include "server.h"
#include "statics.h"

struct epoll_event Server::events[EPOLL_QUEUE_LEN], Server::event;
int numClients;

Server::Server(QObject *parent) : QThread(parent)
{
    qDebug() << "Inside server";
    numClients = 0;
    port = SERVER_PORT;
    addr_size = sizeof(struct sockaddr_in);

    thrdInfo = new thrdParams();

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

void Server::run()
{

    // Execute the epoll Server::event loop
    while(true)
    {
        //struct epoll_Server::event Server::events[MAX_Server::eventS];
        num_fds = epoll_wait (epoll_fd, Server::events, EPOLL_QUEUE_LEN, -1);

        if (num_fds < 0)
            SystemFatal ("Error in epoll_wait!");

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
            assert (Server::events[i].events & EPOLLIN);


            // Case 2: Server is receiving a connection request
            if (Server::events[i].data.fd == fd_server)
            {
                socklen_t addr_size = sizeof(remote_addr);
                fd_new = accept (fd_server, (struct sockaddr*) &remote_addr, &addr_size);
                if (fd_new == -1)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        perror("accept");
                    }
                    continue;
                }

                // Make the fd_new non-blocking
                if (fcntl (fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1)
                    SystemFatal("fcntl");

                // Add the new socket descriptor to the epoll loop
                Server::event.data.fd = fd_new;
                if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_new, &Server::event) == -1)
                    SystemFatal ("epoll_ctl");



                //std::cout << " Remote Address: " << inet_ntoa(remote_addr.sin_addr) << std::endl;
                numClients++;

                //std::cout << "Clients Connected: " << numClients << std::endl;
                continue;
            }

            if (Server::events[i].events & (EPOLLIN))
            {
                // Case 3: One of the sockets has read data
                thrdInfo->fd = Server::events[i].data.fd;

                QFuture<void> future = QtConcurrent::run(readSocket, (void*)thrdInfo);
                future.waitForFinished();

                //ClearSocket((void*)thrdInfo);
              //  pthread_create(&readThread, NULL, &readSocket, (void*)thrdInfo);
              //  pthread_join(readThread, NULL);

            }
            if ( Server::events[i].events & (EPOLLRDHUP))
            {
                close(Server::events[i].data.fd);
                numClients--;
                //std::cout << "Clients Connected: " << numClients <<  std::endl;
            }
            //std::cout << "\r" << "Clients Connected: " << numClients <<  std::flush;

        }


    }
    //exec();
}

void Server::SystemFatal(const char *message)
{
    std::cout << message << std::endl;
      exit (EXIT_FAILURE);
}

void* Server::ClearSocket(void *param)
{
    int	n, bytes_to_read;
        char	*bp, buf[BUFLEN];
        thrdParams* data = (thrdParams*) param;
        int fd = data->fd;

        bp = buf;
        bytes_to_read = BUFLEN;
        while ((n = recv (fd, bp, bytes_to_read, 0)) < BUFLEN)
        {
            bp += n;
            bytes_to_read -= n;


            if (n == 0)
            {
                numClients--;
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
                    numClients--;
                    break;
                  //  pthread_exit(0);
                }
                continue;
            }

        }
        //std::cout << "Sending: " << buf << std::endl;
        /*n = recv (fd, bp, bytes_to_read, 0);

        if (n == 0)
        {
            //qDebug() << "Client Disconnect";
            //close(fd);
            //app->ClientDisconnect();
            thrdCount--;
            pthread_exit(0);
        }
        if (n == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                thrdCount--;
                pthread_exit(0);
            }
        }*/

        send (fd, buf, BUFLEN, 0);
               // pthread_exit(0); // Return n bytes read immediately we dont want this thread running forever.



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
        sleep(1);
        std::cout << "Clients Connected: " << numClients <<  std::endl;
    }
}


void *readSocket(void *param)
{
    int	n, bytes_to_read;
        char	*bp, buf[BUFLEN];
        thrdParams* data = (thrdParams*) param;
        int fd = data->fd;

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

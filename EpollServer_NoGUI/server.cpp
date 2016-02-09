#include "server.h"



//Globals
int fd_server;
int thrdCount;
int numClients;
static struct epoll_event events[EPOLL_QUEUE_LEN], event;
int num_fds, fd_new, epoll_fd;


int main()
{
    int arg;
    int port = SERVER_PORT;
    struct sockaddr_in addr, remote_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sigaction act;
    pthread_t updateConsoleThrd;
    pthread_t workerThrds[NUM_WORKERS];

    thrdCount = 0;
    numClients = 0;

    pthread_create(&updateConsoleThrd, NULL, &UpdateConsole, (void*)1);

    // set up the signal handler to close the server socket when CTRL-c is received
    act.sa_handler = close_fd;
    act.sa_flags = 0;
    if ((sigemptyset (&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1))
    {
            perror ("Failed to set SIGINT handler");
            exit (EXIT_FAILURE);
    }

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

    // Add the server socket to the epoll event loop
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;
    event.data.fd = fd_server;
    if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_server, &event) == -1)
        SystemFatal("epoll_ctl");

    for (int i = 0; i< NUM_WORKERS; i++)
    {
      pthread_create(&workerThrds[i], NULL, &Worker, (void*)1);
      pthread_join(workerThrds[i], NULL);
    }

    close(fd_server);
    exit (EXIT_SUCCESS);
}


void* Worker(void *param)
{
  thrdParams *thrdInfo = new thrdParams();
  struct sockaddr_in remote_addr;
  pthread_t readThread;
  // Execute the epoll event loop
  while (TRUE)
  {
      //struct epoll_event events[MAX_EVENTS];
      num_fds = epoll_wait (epoll_fd, events, EPOLL_QUEUE_LEN, -1);

      if (num_fds < 0)
          SystemFatal ("Error in epoll_wait!");

      for (int i = 0; i < num_fds; i++)
      {
          // Case 1: Error condition
          if (events[i].events & (EPOLLHUP | EPOLLERR))
          {
              numClients--;
              std::cout << "EPOLL ERROR" << std::endl;
              close(events[i].data.fd);
              continue;
          }
          assert (events[i].events & EPOLLIN);


          // Case 2: Server is receiving a connection request
          if (events[i].data.fd == fd_server)
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
              event.data.fd = fd_new;
              if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1)
                  SystemFatal ("epoll_ctl");



              //std::cout << " Remote Address: " << inet_ntoa(remote_addr.sin_addr) << std::endl;
              numClients++;

              //std::cout << "\r" << "Clients Connected: " << numClients << std::flush;
              continue;
          }

          if (events[i].events & (EPOLLIN))
          {
              // Case 3: One of the sockets has read data
              thrdInfo->fd = events[i].data.fd;
              ClearSocket((void*)thrdInfo);
            //  pthread_create(&readThread, NULL, &ClearSocket, (void*)thrdInfo);
            //  pthread_join(readThread, NULL);

          }
          if ( events[i].events & (EPOLLRDHUP))
          {
              close(events[i].data.fd);
              numClients--;
              //std::cout << "\r" << "Clients Connected: " << numClients <<  std::flush;
          }
          //std::cout << "\r" << "Clients Connected: " << numClients <<  std::flush;

      }
  }
}

void* ClearSocket (void* param)
{
    int	n, bytes_to_read;
    char	*bp, buf[BUFLEN];
    thrdParams* data = (thrdParams*) param;
    int fd = data->fd;

    bp = buf;
    bytes_to_read = BUFLEN;
  /*  while ((n = recv (fd, bp, bytes_to_read, 0)) < BUFLEN)
    {
        bp += n;
        bytes_to_read -= n;


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
            continue;
        }

    }*/
    //std::cout << "Sending: " << buf << std::endl;
    n = recv (fd, bp, bytes_to_read, 0);

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
    }

    send (fd, buf, BUFLEN, 0);
    thrdCount--;
    pthread_exit(0); // Return n bytes read immediately we dont want this thread running forever.



}

// Prints the error stored in errno and aborts the program.
void SystemFatal(const char* message)
{
    std::cout << message << std::endl;
    exit (EXIT_FAILURE);
}

// close fd
void close_fd (int signo)
{
    close(fd_server);
    exit (EXIT_SUCCESS);
}

void* UpdateConsole(void* param)
{
  while(TRUE)
  {
    sleep(1);
    std::cout << "Clients Connected: " << numClients <<  std::endl;
  }
}

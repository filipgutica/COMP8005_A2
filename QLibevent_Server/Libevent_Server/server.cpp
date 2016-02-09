#include "server.h"

server::server()
{
    int listen_fd;
    struct sockaddr_in listen_addr;
    int reuseaddr_on = 1;

    /* The socket accept event. */
    struct event ev_accept;

    /* Initialize libevent. */
    event_init();

    /* Create our listening socket. This is largely boiler plate
     * code that I'll abstract away in the future. */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        err(1, "listen failed");
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
        sizeof(reuseaddr_on)) == -1)
        err(1, "setsockopt failed");
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(SERVER_PORT);
    if (bind(listen_fd, (struct sockaddr *)&listen_addr,
        sizeof(listen_addr)) < 0)
        err(1, "bind failed");
    if (listen(listen_fd, 5) < 0)
        err(1, "listen failed");

    /* Set the socket to non-blocking, this is essential in event
     * based programming with libevent. */
    if (setnonblock(listen_fd) < 0)
        err(1, "failed to set server socket to non-blocking");

    /* We now have a listening socket, we create a read event to
     * be notified when a client connects. */
    event_set(&ev_accept, listen_fd, EV_READ|EV_PERSIST, this->on_accept, NULL);
    event_add(&ev_accept, NULL);

    /* Start the libevent event loop. */
    event_dispatch();
}

int server::setnonblock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        return -1;

        return 0;
}

void server::on_read(int fd, short ev, void *arg)
{
    struct client *client = (struct client *)arg;
    struct bufferq *bufferq;
    u_char *buf;
    int len;

    /* Because we are event based and need to be told when we can
     * write, we have to malloc the read buffer and put it on the
     * clients write queue. */
    buf = (u_char*)malloc(BUFLEN);
    if (buf == NULL)
        err(1, "malloc failed");

    len = read(fd, buf, BUFLEN);
    if (len == 0) {
        /* Client disconnected, remove the read event and the
         * free the client structure. */
        printf("Client disconnected.\n");
                close(fd);
        event_del(&client->ev_read);
        free(client);
        return;
    }
    else if (len < 0) {
        /* Some other error occurred, close the socket, remove
         * the event and free the client structure. */
        printf("Socket failure, disconnecting client: %s",
            strerror(errno));
        close(fd);
        event_del(&client->ev_read);
        free(client);
        return;
    }

    /* We can't just write the buffer back as we need to be told
     * when we can write by libevent.  Put the buffer on the
     * client's write queue and schedule a write event. */
    bufferq = (struct bufferq *)calloc(1, sizeof(*bufferq));
    if (bufferq == NULL)
        err(1, "malloc faild");
    bufferq->buf = buf;
    bufferq->len = len;
    bufferq->offset = 0;
    TAILQ_INSERT_TAIL(&client->writeq, bufferq, entries);

    /* Since we now have data that needs to be written back to the
     * client, add a write event. */
    event_add(&client->ev_write, NULL);
}

void server::on_write(int fd, short ev, void *arg)
{
    struct client *client = (struct client *)arg;
    struct bufferq *bufferq;
    int len;

    /* Pull the first item off of the write queue. We probably
     * should never see an empty write queue, but make sure the
     * item returned is not NULL. */
    bufferq = TAILQ_FIRST(&client->writeq);
    if (bufferq == NULL)
        return;

    /* Write the buffer.  A portion of the buffer may have been
     * written in a previous write, so only write the remaining
     * bytes. */
        len = bufferq->len - bufferq->offset;
    len = write(fd, bufferq->buf + bufferq->offset,
                    bufferq->len - bufferq->offset);
    if (len == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            /* The write was interrupted by a signal or we
             * were not able to write any data to it,
             * reschedule and return. */
            event_add(&client->ev_write, NULL);
            return;
        }
        else {
            /* Some other socket error occurred, exit. */
            err(1, "write");
        }
    }
    else if ((bufferq->offset + len) < bufferq->len) {
        /* Not all the data was written, update the offset and
         * reschedule the write event. */
        bufferq->offset += len;
        event_add(&client->ev_write, NULL);
        return;
    }

    /* The data was completely written, remove the buffer from the
     * write queue. */
    TAILQ_REMOVE(&client->writeq, bufferq, entries);
    free(bufferq->buf);
    free(bufferq);
}

void server::on_accept(int fd, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct client *client;

    /* Accept the new connection. */
    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        warn("accept failed");
        return;
    }

    /* Set the client socket to non-blocking mode. */
    if (setnonblock(client_fd) < 0)
        warn("failed to set client socket non-blocking");

    /* We've accepted a new client, allocate a client object to
     * maintain the state of this client. */
    client = (struct client*)calloc(1, sizeof(*client));
    if (client == NULL)
        err(1, "malloc failed");

    /* Setup the read event, libevent will call on_read() whenever
     * the clients socket becomes read ready.  We also make the
     * read event persistent so we don't have to re-add after each
     * read. */
    event_set(&client->ev_read, client_fd, EV_READ|EV_PERSIST, on_read,
        client);

    /* Setting up the event does not activate, add the event so it
     * becomes active. */
    event_add(&client->ev_read, NULL);

    /* Create the write event, but don't add it until we have
     * something to write. */
    event_set(&client->ev_write, client_fd, EV_WRITE, on_write, client);

    /* Initialize the clients write queue. */
    TAILQ_INIT(&client->writeq);

    printf("Accepted connection from %s\n",
               inet_ntoa(client_addr.sin_addr));
}

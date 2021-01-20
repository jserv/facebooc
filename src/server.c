#ifndef _WIN32
# include <arpa/inet.h>
# include <netinet/in.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/select.h>
# include <sys/types.h>
# include <fcntl.h>
# include <time.h>
# include <unistd.h>
# include <errno.h>
typedef int sockopt_t;
#else
# define FD_SETSIZE 4096
# include <ws2tcpip.h>
# include <fcntl.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/stat.h>
# include <time.h>
# include <unistd.h>
# include <errno.h>
# undef DELETE
# undef close
# define close(x) closesocket(x)
typedef char sockopt_t;
#endif

#include "bs.h"
#include "server.h"

#define GET_TIME                                  \
    time_t t = time(NULL);                        \
    char timebuff[100];                           \
    strftime(timebuff, sizeof(timebuff),          \
             "%c", localtime(&t));

#define LOG_400(addr)                             \
    do {                                          \
        GET_TIME;                                 \
        fprintf(stdout,                           \
                "%s %s 400\n",                    \
                timebuff,                         \
                inet_ntoa(addr->sin_addr));       \
    } while (0)

#define LOG_REQUEST(addr, method, path, status)   \
    do {                                          \
        GET_TIME;                                 \
        fprintf(stdout,                           \
                "%s %s %s %s %d\n",               \
                timebuff,                         \
                inet_ntoa(addr->sin_addr),        \
                method,                           \
                path,                             \
                status);                          \
    } while (0)

char *METHODS[8] = {
    "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT"
};

#if defined(__linux__)
#include <sys/epoll.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#else
#error Unsupported platform
#endif

Server *serverNew(uint16_t port)
{
    Server *server = malloc(sizeof(Server));
    server->port = port;
#if defined(__linux__)
    server->priv = epoll_create1(0);
#elif defined(__APPLE__)
    server->priv = kqueue();
#endif
    server->handlers = NULL;
    return server;
}

void serverDel(Server *server)
{
    if (server->handlers) listDel(server->handlers);
    free(server);

#ifdef _WIN32
    WSACleanup();
#endif
}

void serverAddHandler(Server *server, Handler handler)
{
    HandlerP handlerP = &handler;
    server->handlers = listCons(handlerP, sizeof(HandlerP), server->handlers);
}

static Response *staticHandler(Request *req)
{
    ROUTE(req, "/static/");

    // EXIT ON SHENANIGANS
    if (strstr(req->uri, "../")) return NULL;

    char *filename = req->uri + 1;

    // EXIT ON DIRS
    struct stat sbuff;

    if (stat(filename, &sbuff) < 0 || S_ISDIR(sbuff.st_mode))
        return NULL;

    // EXIT ON NOT FOUND
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    // GET LENGTH
    char *buff;
    char  lens[25];
    size_t len;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    sprintf(lens, "%ld", (long int) len);
    rewind(file);

    // SET BODY
    Response *response = responseNew();

    buff = malloc(sizeof(char) * len);
    fread(buff, sizeof(char), len, file);
    responseSetBody(response, bsNewLen(buff, len));
    fclose(file);
    free(buff);

    // MIME TYPE
    char *mimeType = "text/plain";

    len = bsGetLen(req->uri);

    if (!strncmp(req->uri + len - 4, "html", 4)) mimeType = "text/html";
    else if (!strncmp(req->uri + len - 4, "json", 4)) mimeType = "application/json";
    else if (!strncmp(req->uri + len - 4, "jpeg", 4)) mimeType = "image/jpeg";
    else if (!strncmp(req->uri + len - 3,  "jpg", 3)) mimeType = "image/jpeg";
    else if (!strncmp(req->uri + len - 3,  "gif", 3)) mimeType = "image/gif";
    else if (!strncmp(req->uri + len - 3,  "png", 3)) mimeType = "image/png";
    else if (!strncmp(req->uri + len - 3,  "css", 3)) mimeType = "text/css";
    else if (!strncmp(req->uri + len - 2,   "js", 2)) mimeType = "application/javascript";

    // RESPOND
    responseSetStatus(response, OK);
    responseAddHeader(response, "Content-Type", mimeType);
    responseAddHeader(response, "Content-Length", lens);
    responseAddHeader(response, "Cache-Control", "max-age=2592000");
    return response;
}

void serverAddStaticHandler(Server *server)
{
    serverAddHandler(server, staticHandler);
}

static inline int makeSocket(unsigned int port)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;

    if (sock < 0) {
        fprintf(stderr, "error: failed to create socket\n");
        exit(1);
    }

    {
        sockopt_t optval = 1; /* prevent from address being taken */
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        fprintf(stderr, "error: failed to bind socket to 0.0.0.0:%d\n", port);
        exit(1);
    }

    if (listen(sock, 1) < 0) {
        fprintf(stderr, "error: socket failed to listen\n");
        exit(1);
    }

    return sock;
}

static void setFdNonblocking(int fd)
{
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0)
        perror("fcntl");
}

static void serverAddFd(int epollfd, int fd, int in, int oneshot)
{
#if defined(__linux__)
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLET | EPOLLRDHUP;
    event.events |= in ? EPOLLIN : EPOLLOUT;
    if (oneshot)
        event.events |= EPOLLONESHOT;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
        perror("epoll_ctl");
#elif defined(__APPLE__)
    struct kevent event;
    event.ident = fd;
    event.flags = EV_ADD | EV_ENABLE | EV_CLEAR;
    if (oneshot)
        event.flags |= EV_ONESHOT;
    EV_SET(&event, fd, EVFILT_READ, event.flags, 0, 0, NULL);
    if (kevent(epollfd, &event, 1, NULL, 0, NULL) < 0) {
        perror("kevent");
    }
#endif
    setFdNonblocking(fd);
}

static void resetOneShot(int epollfd, int fd)
{
#if defined(__linux__)
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event) < 0)
        perror("epoll_ctl");
#elif defined(__APPLE__)
    struct kevent event;
    event.ident = fd;
    event.flags = EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT;
    EV_SET(&event, fd, EVFILT_READ, event.flags, 0, 0, NULL);
    if (kevent(epollfd, &event, 1, NULL, 0, NULL) < 0) {
        perror("kevent");
    }
#endif
}

static void serverDelFd(Server *server, int fd)
{
#if defined(__linux__)
    if (epoll_ctl(server->priv, EPOLL_CTL_DEL, fd, NULL) < 0)
        perror("epoll_ctl");
#elif defined(__APPLE__)
    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent(server->priv, &event, 1, NULL, 0, NULL) != -1) {
        perror("kevent");
    }
#endif
}

static inline void handle(Server *server, int fd, struct sockaddr_in *addr)
{
    int  nread;
    char buff[20480];

    if ((nread = recv(fd, buff, sizeof(buff), 0)) < 0) {
	    if (errno == EAGAIN) {
	        resetOneShot(server->priv, fd);
	    }		
	    else {
	        fprintf(stderr, "error: read failed\n");
	    }
    } else if (nread > 0) {
        buff[nread] = '\0';

        Request *req = requestNew(buff);

        if (!req) {
            send(fd, "HTTP/1.0 400 Bad Request\r\n\r\nBad Request", 39, 0);
            LOG_400(addr);
        } else {
            ListCell *handler  = server->handlers;
            Response *response = NULL;

            while (handler && !response) {
                response = (*(HandlerP)handler->value)(req);
                handler  = handler->next;
            }

            if (!response) {
                send(fd, "HTTP/1.0 404 Not Found\r\n\r\nNot Found!", 36, 0);
                LOG_REQUEST(addr, METHODS[req->method], req->path, 404);
            } else {
                LOG_REQUEST(addr, METHODS[req->method], req->path,
                            response->status);

                responseWrite(response, fd);
                responseDel(response);
            }

            requestDel(req);
        }
    }
    serverDelFd(server, fd);
    close(fd);
}

void serverServe(Server *server)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(2 , &wsaData);
#endif

    int sock = makeSocket(server->port);
    int newSock, nfds, tmpfd;
    struct sockaddr_in addr;
#if defined(__linux__)
    struct epoll_event* events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * 64);
    struct epoll_event event;
#elif defined(__APPLE__)
    struct kevent* events = (struct kevent*)malloc(sizeof(struct kevent) * 64);
    struct kevent event;
#endif
    
    socklen_t size;

    serverAddFd(server->priv, sock, 1,0);

    fprintf(stdout, "Listening on port %d.\n\n", server->port);

    for (;;) {
    #if defined(__linux__)
        nfds = epoll_wait(server->priv, events, 64, -1);
    #elif defined(__APPLE__)
        nfds = kevent(server->priv, NULL, 0, events, 64, NULL);
    #endif
	for (int i = 0; i < nfds; ++i) {
            event = events[i];
        #if defined(__linux__)
	    tmpfd = event.data.fd;
        #elif defined(__APPLE__)
            tmpfd = (int)event.ident;
        #endif
	    if (tmpfd == sock) {
	        size = sizeof(addr);
	        while ((newSock = accept(sock, (struct sockaddr *) &addr, &size)) > 0) {
	            serverAddFd(server->priv, newSock, 1, 1);
	    	}
		if (newSock == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR) {
                        fprintf(stderr, "error: failed to accept connection\n");
                        exit(1);
                    }
                }
	    } else {
	        handle(server, tmpfd, &addr);
	    }
	}
    }
}

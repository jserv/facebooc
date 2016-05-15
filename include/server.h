#ifndef SERVER_H
#define SERVER_H

#include <sys/epoll.h>
#include "list.h"
#include "request.h"
#include "response.h"

typedef struct Server {
    unsigned int port;
    int epollfd;
    ListCell *handlers;
} Server;

typedef Response *(*Handler)(Request *);
typedef Response *(**HandlerP)(Request *);

Server *serverNew(unsigned int);
void    serverDel(Server *);
void    serverAddHandler(Server *, Handler);
void    serverAddStaticHandler(Server *);
void    serverServe(Server *);

#endif

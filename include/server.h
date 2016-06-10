#ifndef SERVER_H
#define SERVER_H

#include "list.h"
#include "request.h"
#include "response.h"

typedef struct Server {
    unsigned int port;

    ListCell *handlers;
} Server;

typedef Response *(*Handler)(Request *);
typedef Response *(**HandlerP)(Request *);

Server *serverNew(uint16_t);
void    serverDel(Server *);
void    serverAddHandler(Server *, Handler);
void    serverAddStaticHandler(Server *);
void    serverServe(Server *);

#endif

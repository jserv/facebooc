#ifndef SERVER_H
#define SERVER_H

#include "list.h"
#include "request.h"
#include "response.h"
#include "route.h"

typedef struct Server {
    unsigned int port;

    ListCell *routes;
} Server;

typedef Response *(*Handler)(Request *);
typedef Response *(**HandlerP)(Request *);

Server *serverNew(unsigned int);
void    serverDel(Server *);
Route  *serverAddRoute(Server *, ROUTE_MATCH, const char *, Handler);
void    serverAddStaticRoute(Server *, ROUTE_MATCH, const char *);
void    serverServe(Server *);

#endif

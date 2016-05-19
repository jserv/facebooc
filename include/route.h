#ifndef ROUTE_H
#define ROUTE_H

#include "response.h"
#include "request.h"

typedef enum ROUTE_MATCH {
    NONE, EXACT, NORMAL
} ROUTE_MATCH;

typedef Response *(*Handler)(Request *);
typedef Response *(**HandlerP)(Request *);

typedef struct Route {
    const char * path;
    ROUTE_MATCH match;

    Handler handler;
} Route;

Route    *routeNew(Handler handler);
void      routeDel(Route *);
Response *routeHandle(Route *, Request *);
void      routeAddPath(Route *, ROUTE_MATCH match, const char *);

#endif

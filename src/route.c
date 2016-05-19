#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "route.h"

Route *routeNew(Handler handler)
{
    Route *route = malloc(sizeof(Route));
    route->path = NULL;
    route->match = NONE;
    route->handler = handler;
    return route;
}

void routeDel(Route *route)
{
    free(route);
}

Response *routeHandle(Route *route, Request * req)
{
    if (route->path) {
        switch (route->match) {
            case EXACT:
                if (strcmp(req->uri, route->path) != 0)
                    return NULL;
                break;
            case NORMAL:
                if (strncmp(req->uri, route->path, strlen(route->path)) != 0)
                    return NULL;
                break;
            default:
                break;
        }
    }

    return route->handler(req);
}

void routeAddPath(Route *route, ROUTE_MATCH match, const char *path)
{
    route->path = path;
    route->match = match;
}


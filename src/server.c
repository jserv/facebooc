#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "bs.h"
#include "server.h"

#define log400(addr) {                            \
    time_t t = time(NULL);                        \
    char timebuff[100];                           \
    strftime(timebuff, 100, "%c", localtime(&t)); \
    fprintf(stdout,                               \
            "%s %s 400\n",                        \
            timebuff,                             \
            inet_ntoa(addr->sin_addr));           \
}

#define logRequest(addr, method, path, status) {  \
    time_t t = time(NULL);                        \
    char timebuff[100];                           \
    strftime(timebuff, 100, "%c", localtime(&t)); \
    fprintf(stdout,                               \
            "%s %s %s %s %d\n",                   \
            timebuff,                             \
            inet_ntoa(addr->sin_addr),            \
            method,                               \
            path,                                 \
            status);                              \
}

char *METHODS[8] = {
    "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT"
};

Server *serverNew(unsigned int port) {
    Server *server = malloc(sizeof(Server));

    server->port     = port;
    server->handlers = NULL;

    return server;
}

void serverDel(Server *server) {
    if (server->handlers != NULL) listDel(server->handlers);

    free(server);
}

void serverAddHandler(Server *server, Handler handler) {
    HandlerP handlerP = &handler;

    server->handlers = listCons(handlerP, sizeof(HandlerP), server->handlers);
}

static Response *staticHandler(Request *req) {
    ROUTE(req, "/static/");

    // 404 ON SHENANIGANS
    if (strstr(req->uri, "../") != NULL)
        return NULL;

    FILE *file = fopen(req->uri + 1, "r");

    // 404 ON NOT FOUND
    if (file == NULL)
        return NULL;

    Response *response = responseNew();
    char *buffer, lengthBuffer[25];
    size_t length;
    struct stat statBuffer;

    // 404 ON DIRS
    stat(req->uri + 1, &statBuffer);

    if (S_ISDIR(statBuffer.st_mode)) {
        fclose(file);
        responseDel(response);
        return NULL;
    }

    // GET LENGTH
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    sprintf(lengthBuffer, "%ld", length);
    rewind(file);

    // SET BODY
    buffer = malloc(sizeof(char) * length);
    fread(buffer, sizeof(char), length, file);
    responseSetBody(response, bsNewLen(buffer, length));
    free(buffer);
    fclose(file);

    // RESPOND
    responseSetStatus(response, OK);
    responseAddHeader(response, "Content-Type", "text/plain");
    responseAddHeader(response, "Content-Length", lengthBuffer);
    return response;
}

void serverAddStaticHandler(Server *server) {
    serverAddHandler(server, staticHandler);
}

static inline int makeSocket(unsigned int port) {
   int sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;

    if (sock < 0) {
        fprintf(stderr, "error: failed to create socket\n");
        exit(1);
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

static inline void handle(Server *server, int fd, fd_set *activeFDs, struct sockaddr_in *addr) {
    int  nread;
    char buff[20480];

    if ((nread = read(fd, buff, sizeof(buff))) < 0) {
        fprintf(stderr, "error: read failed\n");
    } else if (nread > 0) {
        Request *req = requestNew(buff);

        if (req == NULL) {
            write(fd, "HTTP/1.0 400 Bad Request\r\n\r\nBad Request", 39);

            log400(addr);
        } else {
            ListCell *handler  = server->handlers;
            Response *response = NULL;

            while (handler != NULL && response == NULL) {
                response = (*(HandlerP)handler->value)(req);
                handler  = handler->next;
            }

            if (response == NULL) {
                write(fd, "HTTP/1.0 404 Not Found\r\n\r\nNot Found!", 36);

                logRequest(addr, METHODS[req->method], req->path, 404);
            } else {
                logRequest(addr, METHODS[req->method], req->path, response->status);

                responseWrite(response, fd);
                responseDel(response);
            }

            requestDel(req);
        }
    }

    close(fd);

    FD_CLR(fd, activeFDs);
}

void serverServe(Server *server) {
    int sock = makeSocket(server->port);
    int newSock;

    socklen_t size;

    fd_set activeFDs;
    fd_set readFDs;

    struct sockaddr_in addr;

    FD_ZERO(&activeFDs);
    FD_SET(sock, &activeFDs);

    fprintf(stdout, "Listening on port %d.\n\n", server->port);

    for (;;) {
        readFDs = activeFDs;

        if (select(FD_SETSIZE, &readFDs, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "error: failed to select\n");
            exit(1);
        }

        for (int fd = 0; fd < FD_SETSIZE; ++fd) {
            if (FD_ISSET(fd, &readFDs)) {
                if (fd == sock) {
                    size    = sizeof(addr);
                    newSock = accept(sock, (struct sockaddr *) &addr, &size);

                    if (newSock < 0) {
                        fprintf(stderr, "error: failed to accept connection\n");
                        exit(1);
                    }

                    FD_SET(newSock, &activeFDs);
                } else {
                    handle(server, fd, &activeFDs, &addr);
                }
            }
        }
    }
}

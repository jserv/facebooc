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
# include <time.h>
# include <unistd.h>
typedef int sockopt_t;
#else
# define FD_SETSIZE 4096
# include <ws2tcpip.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/stat.h>
# include <time.h>
# include <unistd.h>
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

Server *serverNew(uint16_t port)
{
    Server *server = malloc(sizeof(Server));
    server->port = port;
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

static inline void handle(Server *server, int fd, fd_set *activeFDs, struct sockaddr_in *addr)
{
    int  nread;
    char buff[20480];

    if ((nread = recv(fd, buff, sizeof(buff), 0)) < 0) {
        fprintf(stderr, "error: read failed\n");
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

    close(fd);

    FD_CLR(fd, activeFDs);
}

void serverServe(Server *server)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(2 , &wsaData);
#endif

    int sock = makeSocket(server->port);
    int newSock;
    int nfds = sock + 1;

    struct sockaddr_in addr;
    socklen_t size = sizeof(addr);

    fd_set activeFDs;
    fd_set readFDs;

    FD_ZERO(&activeFDs);
    FD_SET(sock, &activeFDs);

    fprintf(stdout, "Listening on port %d.\n\n", server->port);
    for (;;) {
        readFDs = activeFDs;

        if (select(nfds, &readFDs, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "error: failed to select\n");
            exit(1);
        }

        if (FD_ISSET(sock, &readFDs)) {
            newSock = accept(sock, (struct sockaddr *) &addr, &size);

            if (newSock < 0) {
                fprintf(stderr, "error: failed to accept connection\n");
                exit(1);
            }

            if (newSock >= nfds) nfds = newSock + 1;
            FD_SET(newSock, &activeFDs);
        }

        for (int fd = sock + 1; fd < nfds; ++fd) {
            if (FD_ISSET(fd, &readFDs)) handle(server, fd, &activeFDs, &addr);
        }
    }
}

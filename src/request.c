#include <stdio.h>

#include "bs.h"
#include "kv.h"
#include "request.h"

static inline ListCell *parseCookies(char *header) {
    ListCell *cookies = NULL;

    char *copy = bsNew(header);
    char *segment, *key;

    bool s = true;

    for (;;) {
        if (s) {segment = strtok(copy, "="); s = false;}
        else   {segment = strtok(NULL, "=");}

        if (segment == NULL) break;

        if (*segment == ' ') segment += 1;

        key     = segment;
        segment = strtok(NULL, ";\0");

        if (segment == NULL) break;

        cookies = listCons(kvNew(key, segment), sizeof(KV), cookies);
    }

    bsDel(copy);

    return cookies;
}

static inline ListCell *parseQS(char *path) {
    ListCell *qs = NULL;

    char *copy = bsNew(path);
    char *segment, *key;

    bool s = true;

    for (;;) {
        if (s) {segment = strtok(copy, "="); s = false;}
        else   {segment = strtok(NULL, "=");}

        if (segment == NULL) break; 

        key     = segment;
        segment = strtok(NULL, "&\0");

        if (segment == NULL) break;

        qs = listCons(kvNew(key, segment), sizeof(KV), qs);
    }

    bsDel(copy);

    return qs;
}

static inline ListCell *parseHeaders(char *segment) {
    ListCell *headers = NULL;

    char *header;

    while (*segment != '\r' && segment != NULL) {
        segment = strtok(NULL, ":");

        if (segment == NULL)
            break;

        header  = segment;
        segment = strtok(NULL, "\n");

        if (segment == NULL)
            break;

        if (*segment == ' ')
            segment += 1;

        headers = listCons(kvNew(header, segment), sizeof(KV), headers);
    }

    return headers;
}

#define TOK(s, d)           \
    segment = strtok(s, d); \
    if (segment == NULL)    \
        goto fail;

Request *requestNew(char *buff) {
    Request *request = malloc(sizeof(Request));

    char *segment;

    request->method      = UNKNOWN_METHOD;
    request->path        = NULL;
    request->uri         = NULL;
    request->queryString = NULL;
    request->headers     = NULL;
    request->cookies     = NULL;

    // METHOD
    TOK(buff, " \t");

    if      (strcmp(segment, "OPTIONS") == 0) request->method = OPTIONS;
    else if (strcmp(segment, "GET")     == 0) request->method = GET;
    else if (strcmp(segment, "HEAD")    == 0) request->method = HEAD;
    else if (strcmp(segment, "POST")    == 0) request->method = POST;
    else if (strcmp(segment, "PUT")     == 0) request->method = PUT;
    else if (strcmp(segment, "DELETE")  == 0) request->method = DELETE;
    else if (strcmp(segment, "TRACE")   == 0) request->method = TRACE;
    else if (strcmp(segment, "CONNECT") == 0) request->method = CONNECT;
    else goto fail;

    // PATH
    TOK(NULL, " \t");

    request->path = bsNew(segment);
    request->uri  = bsNew(segment);

    if (strchr(request->path, '#') != NULL)
        goto fail;

    // VERSION
    TOK(NULL, "\n");

    if (strncmp(segment, "HTTP/1.0", 8) != 0 &&
        strncmp(segment, "HTTP/1.1", 8) != 0)
        goto fail;

    // HEADERS
    request->headers = parseHeaders(segment);

    // TODO: BODY

    // QUERYSTRING
    segment = strchr(request->path, '?');

    if (segment != NULL) {
        request->uri = bsNewLen(request->path, segment - request->path);
        request->queryString = parseQS(segment + 1);

        if (request->queryString == NULL)
            goto fail;
    }

    // COOKIES
    segment = kvFindList(request->headers, "Cookie");

    if (segment != NULL) {
        request->cookies = parseCookies(segment);

        if (request->cookies == NULL)
            goto fail;
    }

    return request;

fail:
    requestDel(request);

    return NULL;
}

void requestDel(Request *req) {
    if (req->path        != NULL) bsDel(req->path);
    if (req->uri         != NULL) bsDel(req->uri);
    if (req->queryString != NULL) kvDelList(req->queryString);
    if (req->headers     != NULL) kvDelList(req->headers);
    if (req->cookies     != NULL) kvDelList(req->cookies);

    free(req);
}

#include <stdio.h>

#include "bs.h"
#include "kv.h"
#include "request.h"

// TODO: Make this less shitty.
static inline char *urldecode(char *segment) {
    char *cc  = segment;
    char *bs  = bsNew("");
    char c[2] = "\0\0";

    while (*cc != '\0') {
        if (*cc == '+') *cc = ' ';
        if (*cc == '%') {
            *cc = '\0';

            bsLCat(&bs, segment);

            if (*(cc + 1) == '0' && *(cc + 2) == 'D' &&
                *(cc + 3) == '%' && *(cc + 4) == '0' &&
                *(cc + 5) == 'A') {
                bsLCat(&bs, "\n");

                cc += 5;
                segment = cc + 1;
            } else {
                c[0] = (char)strtol(cc + 1, &segment, 16);
                cc   = segment;

                bsLCat(&bs, c);
            }
        }

        cc++;
    }

    bsLCat(&bs, segment);

    return bs;
}

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
    char *segment, *key, *value;

    bool s = true;

    for (;;) {
        if (s) {segment = strtok(copy, "="); s = false;}
        else   {segment = strtok(NULL, "=");}

        if (segment == NULL) break;
        if (*(segment + strlen(segment) + 1) == '&') continue;

        key     = segment;
        segment = strtok(NULL, "&\0");

        if (segment == NULL) break;

        key   = urldecode(key);
        value = urldecode(segment);
        qs    = listCons(kvNew(key, value), sizeof(KV), qs);

        bsDel(key);
        bsDel(value);
    }

    bsDel(copy);

    return qs;
}

static inline ListCell *parseHeaders(char *segment) {
    ListCell *headers = NULL;

    size_t len;
    char *header;

    while (segment != NULL) {
        segment = strtok(NULL, ":\n");

        if (segment == NULL || *segment == '\r')
            break;

        header  = segment;
        segment = strtok(NULL, "\n");

        if (segment == NULL)
            break;

        if (*segment == ' ')
            segment += 1;

        len = strlen(segment);

        if (*(segment + len - 1) == '\r')
            *(segment + len - 1)  = '\0';

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

    char *segment, *bs;

    request->method      = UNKNOWN_METHOD;
    request->path        = NULL;
    request->uri         = NULL;
    request->queryString = NULL;
    request->postBody    = NULL;
    request->headers     = NULL;
    request->cookies     = NULL;
    request->account     = NULL;

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

    // BODY
    bs = kvFindList(request->headers, "Content-Type");

    if (bs != NULL && strncmp(bs, "application/x-www-form-urlencoded", 33) == 0) {
        segment = strtok(NULL, "\0");

        if (segment == NULL)
            goto fail;

        request->postBody = parseQS(segment);
    }

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
    if (req->postBody    != NULL) kvDelList(req->postBody);
    if (req->headers     != NULL) kvDelList(req->headers);
    if (req->cookies     != NULL) kvDelList(req->cookies);
    if (req->account     != NULL) accountDel(req->account);

    free(req);
}

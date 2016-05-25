#include <stdio.h>

#include "bs.h"
#include "kv.h"
#include "request.h"

// TODO: Make this less shitty.
static inline char *urldecode(char *segment)
{
    char *cc   = segment;
    char *bs   = bsNew("");
    char escCode[3] = "\0\0\0";
    char escChar[2] = "\0\0";

    while (*cc != '\0') {
        switch (*cc) {
            case '+': 
                *cc++ = ' ';
                break;
            case '%': 
                *cc++ = '\0';
                bsLCat(&bs, segment);
                escCode[0] = *cc++;
                escCode[1] = *cc++;
                escChar[0] = (char) strtol(escCode, NULL, 16);

                segment = cc;

                bsLCat(&bs, escChar);
                break;
            default:
                cc++;
        }
    }
    bsLCat(&bs, segment);

    return bs;
}

static inline ListCell *parseCookies(char *header)
{
    ListCell *cookies = NULL;
    char *copy = bsNew(header);
    char *segment, *key;

    bool s = true;

    for (;;) {
        if (s) {
            segment = strtok(copy, "=");
            s = false;
        } else   {
            segment = strtok(NULL, "=");
        }

        if (!segment) break;

        if (*segment == ' ') segment += 1;

        key     = segment;
        segment = strtok(NULL, ";\0");

        if (!segment) break;

        cookies = listCons(kvNew(key, segment), sizeof(KV), cookies);
    }

    bsDel(copy);

    return cookies;
}

static inline ListCell *parseQS(char *path)
{
    ListCell *qs = NULL;
    char *copy = bsNew(path);
    char *segment, *key, *value;

    bool s = true;

    for (;;) {
        if (s) {
            segment = strtok(copy, "=");
            s = false;
        } else   {
            segment = strtok(NULL, "=");
        }

        if (!segment) break;
        if (*(segment + strlen(segment) + 1) == '&') continue;

        key     = segment;
        segment = strtok(NULL, "&\0");

        if (!segment) break;

        key   = urldecode(key);
        value = urldecode(segment);
        qs    = listCons(kvNew(key, value), sizeof(KV), qs);

        bsDel(key);
        bsDel(value);
    }

    bsDel(copy);

    return qs;
}

static inline ListCell *parseHeaders(char *segment)
{
    ListCell *headers = NULL;
    size_t len;
    char *header;

    while (segment) {
        segment = strtok(NULL, ":\n");
        if (!segment || *segment == '\r') break;

        header  = segment;
        segment = strtok(NULL, "\n");

        if (!segment) break;

        if (*segment == ' ')
            segment += 1;

        len = strlen(segment);

        if (*(segment + len - 1) == '\r')
            *(segment + len - 1)  = '\0';

        headers = listCons(kvNew(header, segment), sizeof(KV), headers);
    }

    return headers;
}

#define TOK(s, d)               \
    segment = strtok(s, d);     \
    if (!segment)               \
        goto fail;

Request *requestNew(char *buff)
{
    Request *request = malloc(sizeof(Request));

    char *segment, *bs;

    request->method = UNKNOWN_METHOD;
    request->path = NULL;
    request->uri = NULL;
    request->queryString = NULL;
    request->postBody = NULL;
    request->headers = NULL;
    request->cookies = NULL;
    request->account = NULL;

    // METHOD
    TOK(buff, " \t");

    if (!strcmp(segment, "OPTIONS")) request->method = OPTIONS;
    else if (!strcmp(segment, "GET")) request->method = GET;
    else if (!strcmp(segment, "HEAD")) request->method = HEAD;
    else if (!strcmp(segment, "POST")) request->method = POST;
    else if (!strcmp(segment, "PUT")) request->method = PUT;
    else if (!strcmp(segment, "DELETE")) request->method = DELETE;
    else if (!strcmp(segment, "TRACE")) request->method = TRACE;
    else if (!strcmp(segment, "CONNECT")) request->method = CONNECT;
    else goto fail;

    // PATH
    TOK(NULL, " \t");

    request->path = bsNew(segment);
    request->uri  = bsNew(segment);

    if (strchr(request->path, '#')) goto fail;

    // VERSION
    TOK(NULL, "\n");

    if (strncmp(segment, "HTTP/1.0", 8) != 0 &&
        strncmp(segment, "HTTP/1.1", 8) != 0)
        goto fail;

    // HEADERS
    request->headers = parseHeaders(segment);

    // BODY
    bs = kvFindList(request->headers, "Content-Type");

    if (bs && !strncmp(bs, "application/x-www-form-urlencoded", 33)) {
        segment = strtok(NULL, "\0");
        if (!segment) goto fail;

        request->postBody = parseQS(segment);
    }

    // QUERYSTRING
    segment = strchr(request->path, '?');
    if (segment) {
        request->uri = bsNewLen(request->path, segment - request->path);
        request->queryString = parseQS(segment + 1);
        if (!request->queryString) goto fail;
    }

    // COOKIES
    segment = kvFindList(request->headers, "Cookie");

    if (segment) {
        request->cookies = parseCookies(segment);
        if (!request->cookies) goto fail;
    }

    return request;

fail:
    requestDel(request);

    return NULL;
}

void requestDel(Request *req)
{
    if (req->path) bsDel(req->path);
    if (req->uri) bsDel(req->uri);
    if (req->queryString) kvDelList(req->queryString);
    if (req->postBody) kvDelList(req->postBody);
    if (req->headers) kvDelList(req->headers);
    if (req->cookies) kvDelList(req->cookies);
    if (req->account) accountDel(req->account);

    free(req);
}

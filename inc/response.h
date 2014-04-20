#ifndef RESPONSE_H
#define RESPONSE_H

#include "list.h"

typedef enum Status {
    CONTINUE              = 100, SWITCHING_PROTOCOLS,
    OK                    = 200, CREATED, ACCEPTED, NAI, NO_CONTENT, RESET_CONTENT, PARTIAL_CONTENT,
    MULTIPLE_CHOICES      = 300, MOVED_PERMANENTLY, FOUND, SEE_OTHER, NOT_MODIFIED, USE_PROXY, SWITCH_PROXY, TEMPORARY_REDIRECT, PERMANENT_REDIRECT,
    BAD_REQUEST           = 400, UNAUTHORIZED, PAYMENT_REQUIRED, FORBIDDEN, NOT_FOUND, METHOD_NOT_ALLOWED, NOT_ACCEPTABLE, PROXY_AUTH_REQUIRED, REQUEST_TIMEOUT, CONFLICT, GONE, LENGTH_REQUIRED, PRECONDITION_FAILED, REQUEST_ENTITY_TOO_LARGE, REQUEST_URI_TOO_LONG, UNSUPPORTED_MEDIA_TYPE, REQUESTED_RANGE_NOT_SATISFIABLE, EXPECTATION_FAILED, IM_A_TEAPOT, AUTHENTICATION_TIMEOUT, ENHANCE_YOUR_CALM,
    INTERNAL_SERVER_ERROR = 500, NOT_IMPLEMENTED, BAD_GATEWAY, SERVICE_UNAVAILABLE, GATEWAY_TIMEOUT, HTTP_VERSION_NOT_SUPPORTED
} Status;

typedef struct Response {
    Status status;

    ListCell *headers;

    char *body;
} Response;

Response *responseNew();
Response *responseNewRedirect(char *);
void      responseSetStatus(Response *, Status);
void      responseSetBody(Response *, char *);
void      responseAddCookie(Response *, char *, char *, char *, char *, int);
void      responseAddHeader(Response *, char *, char *);
void      responseDel(Response *);
void      responseWrite(Response *, int);

#endif

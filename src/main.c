#include <stdio.h>

#include "bs.h"
#include "server.h"

Response *hello(Request *req) {
    EXACT_ROUTE(req, "/");

    Response *response = responseNew();
    responseSetStatus(response, OK);
    responseSetBody(response, bsNew("Hello, World!\r\n"));
    return response;
}

int main(void) {
    Server *server = serverNew(8091);
    serverAddStaticHandler(server);
    serverAddHandler(server, hello);
    serverServe(server);
    serverDel(server);

    return 0;
}

#include <stdio.h>

#include "bs.h"
#include "server.h"
#include "template.h"

Response *hello(Request *req) {
    EXACT_ROUTE(req, "/");

    Response *response = responseNew();
    Template *template = templateNew("templates/index.html");
    responseSetStatus(response, OK);
    templateSet(template, "title", "Home");
    templateSet(template, "username", "Bogdan");
    responseSetBody(response, templateRender(template));
    templateDel(template);
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

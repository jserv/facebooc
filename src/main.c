#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>

#include "bs.h"
#include "server.h"
#include "template.h"

// INITIALIZATION
// ==============

Server  *server = NULL;
sqlite3 *DB     = NULL;

static void sig(int signum) {
    if (server != NULL) serverDel(server);
    if (DB     != NULL) sqlite3_close(DB);

    fprintf(stdout, "\n[%d] Buh-bye!\n", signum);
    exit(0);
}

static void createDB(const char *e) {
    char *err;

    if (sqlite3_exec(DB, e, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "error: initDB: %s\n", err);
        sqlite3_free(err);
        exit(1);
    }
}

static void initDB() {
    if (sqlite3_open("db.sqlite3", &DB)) {
        fprintf(stderr, "error: unable to open DB: %s\n", sqlite3_errmsg(DB));
        exit(1);
    }

    createDB("CREATE TABLE IF NOT EXISTS accounts ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", name      TEXT"
             ", username  TEXT"
             ", email     TEXT UNIQUE"
             ", password  TEXT"
             ")");

    createDB("CREATE TABLE IF NOT EXISTS sessions ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", account   INTEGER"
             ", session   TEXT"
             ")");

    createDB("CREATE TABLE IF NOT EXISTS posts ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", author    INTEGER"
             ", title     TEXT"
             ", body      TEXT"
             ")");

    createDB("CREATE TABLE IF NOT EXISTS likes ("
             "  id     INTEGER PRIMARY KEY ASC"
             ", author INTEGER"
             ", post   INTEGER"
             ")");
}

static Response *hello(Request *);

int main(void) {
    if (signal(SIGINT,  sig) == SIG_ERR ||
        signal(SIGTERM, sig) == SIG_ERR) {
        fprintf(stderr, "error: failed to bind signal handler\n");
        return 1;
    }

    initDB();

    Server *server = serverNew(8091);
    serverAddStaticHandler(server);
    serverAddHandler(server, hello);
    serverServe(server);

    return 0;
}

// HANDLERS
// ========

static Response *hello(Request *req) {
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

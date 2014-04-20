#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <time.h>

#include "bs.h"
#include "kv.h"
#include "server.h"
#include "template.h"

#include "models/account.h"
#include "models/session.h"

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

static Response *home(Request *);
static Response *dashboard(Request *);
static Response *login(Request *);
static Response *logout(Request *);
static Response *signup(Request *);
static Response *about(Request *);
static Response *notFound(Request *);

int main(void) {
    if (signal(SIGINT,  sig) == SIG_ERR ||
        signal(SIGTERM, sig) == SIG_ERR) {
        fprintf(stderr, "error: failed to bind signal handler\n");
        return 1;
    }

    srand(time(NULL));

    initDB();

    Server *server = serverNew(8091);
    serverAddHandler(server, notFound);
    serverAddStaticHandler(server);
    serverAddHandler(server, about);
    serverAddHandler(server, signup);
    serverAddHandler(server, logout);
    serverAddHandler(server, login);
    serverAddHandler(server, dashboard);
    serverAddHandler(server, home);
    serverServe(server);

    return 0;
}

// HANDLERS
// ========

#define invalid(k, v) {          \
    templateSet(template, k, v); \
    valid = false;               \
}

static Response *home(Request *req) {
    EXACT_ROUTE(req, "/");

    Response *response = responseNew();
    Template *template = templateNew("templates/index.html");
    responseSetStatus(response, OK);
    templateSet(template, "subtitle", "Dashboard");
    templateSet(template, "username", "Bogdan");
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *dashboard(Request *req) {
    EXACT_ROUTE(req, "/");

    Response *response = responseNew();
    Template *template = templateNew("templates/index.html");
    responseSetStatus(response, OK);
    templateSet(template, "subtitle", "Dashboard");
    templateSet(template, "username", "Bogdan");
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *login(Request *req) {
    EXACT_ROUTE(req, "/login/");

    Response *response = responseNew();
    Template *template = templateNew("templates/login.html");
    responseSetStatus(response, OK);
    templateSet(template, "subtitle", "Login");

    if (req->method == POST) {
        bool valid = true;

        char *username = kvFindList(req->postBody, "username");
        char *password = kvFindList(req->postBody, "password");

        if (username == NULL) {
            invalid("usernameError", "Username missing!");
        } else {
            templateSet(template, "formUsername", username);
        }

        if (password == NULL) {
            invalid("passwordError", "Password missing!");
        }

        if (valid) {
            Session *session = sessionCreate(DB, username, password);

            if (session != NULL) {
                responseSetStatus(response, FOUND);
                responseAddCookie(response, "sid", session->sessionId, NULL, NULL, 3600 * 24 * 30);
                responseAddHeader(response, "Location", "/dashboard/");
                templateDel(template);
                sessionDel(session);
                return response;
            } else {
                invalid("usernameError", "Invalid username or password.");
            }
        }
    }

    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *logout(Request *req) {
    EXACT_ROUTE(req, "/logout/");

    Response *response = responseNew();
    responseSetStatus(response, FOUND);
    responseAddCookie(response, "sid", "", NULL, NULL, -1);
    responseAddHeader(response, "Location", "/");
    return response;
}

static Response *signup(Request *req) {
    EXACT_ROUTE(req, "/signup/");

    Response *response = responseNew();
    Template *template = templateNew("templates/signup.html");
    templateSet(template, "subtitle", "Sign Up");
    responseSetStatus(response, OK);

    if (req->method == POST) {
        bool valid = true;
        char *name = kvFindList(req->postBody, "name");
        char *email = kvFindList(req->postBody, "email");
        char *username = kvFindList(req->postBody, "username");
        char *password = kvFindList(req->postBody, "password");
        char *confirmPassword = kvFindList(req->postBody, "confirm-password");

        if (name == NULL) {
            invalid("nameError", "You must enter your name!");
        } else if (strlen(name) < 5 || strlen(name) > 50) {
            invalid("nameError", "Your name must be between 5 and 50 characters long.");
        } else {
            templateSet(template, "formName", name);
        }

        if (email == NULL) {
            invalid("emailError", "You must enter an email!");
        } else if (strchr(email, '@') == NULL) {
            invalid("emailError", "Invalid email.");
        } else if (strlen(email) < 3 || strlen(email) > 50) {
            invalid("emailError", "Your email must be between 3 and 50 characters long.");
        } else if (!accountCheckEmail(DB, email)) {
            invalid("emailError", "This email is taken.");
        } else {
            templateSet(template, "formEmail", email);
        }

        if (username == NULL) {
            invalid("usernameError", "You must enter a username!");
        } else if (strlen(username) < 3 || strlen(username) > 50) {
            invalid("usernameError", "Your username must be between 3 and 50 characters long.");
        } else if (!accountCheckUsername(DB, username)) {
            invalid("usernameError", "This username is taken.");
        } else {
            templateSet(template, "formUsername", username);
        }

        if (password == NULL) {
            invalid("passwordError", "You must enter a password!");
        } else if (strlen(password) < 8) {
            invalid("passwordError", "Your password must be at least 8 characters long!");
        }

        if (confirmPassword == NULL) {
            invalid("confirmPasswordError", "You must confirm your password.");
        } else if (strcmp(password, confirmPassword) != 0) {
            invalid("confirmPasswordError", "The two passwords must be the same.");
        }

        if (valid) {
            Account *account = accountCreate(DB, name, email, username, password);

            if (account != NULL) {
                responseSetStatus(response, FOUND);
                responseAddHeader(response, "Location", "/login/");
                templateDel(template);
                accountDel(account);
                return response;
            } else {
                invalid("nameError", "Unexpected error. Please try again later.");
            }
        }
    }

    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *about(Request *req) {
    EXACT_ROUTE(req, "/about/");

    Response *response = responseNew();
    Template *template = templateNew("templates/about.html");
    templateSet(template, "subtitle", "About");
    responseSetStatus(response, OK);
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *notFound(Request *req) {
    Response *response = responseNew();
    Template *template = templateNew("templates/404.html");
    templateSet(template, "subtitle", "404 Not Found");
    responseSetStatus(response, NOT_FOUND);
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

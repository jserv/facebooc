#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bs.h"
#include "kv.h"
#include "server.h"
#include "template.h"

#include "models/account.h"
#include "models/connection.h"
#include "models/like.h"
#include "models/post.h"
#include "models/session.h"

Server  *server = NULL;
sqlite3 *DB = NULL;

static void sig(int signum)
{
    if (server) serverDel(server);
    if (DB) sqlite3_close(DB);

    fprintf(stdout, "\n[%d] Bye!\n", signum);
    exit(0);
}

static void createDB(const char *e)
{
    char *err;

    if (sqlite3_exec(DB, e, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "error: initDB: %s\n", err);
        sqlite3_free(err);
        exit(1);
    }
}

static void initDB()
{
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

    createDB("CREATE TABLE IF NOT EXISTS connections ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", account1  INTEGER"
             ", account2  INTEGER"
             ")");

    createDB("CREATE TABLE IF NOT EXISTS posts ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", author    INTEGER"
             ", body      TEXT"
             ")");

    createDB("CREATE TABLE IF NOT EXISTS likes ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", account   INTEGER"
             ", author    INTEGER"
             ", post      INTEGER"
             ")");
}

static Response *session(Request *);
static Response *home(Request *);
static Response *dashboard(Request *);
static Response *profile(Request *);
static Response *post(Request *);
static Response *like(Request *);
static Response *unlike(Request *);
static Response *connect(Request *);
static Response *search(Request *);
static Response *login(Request *);
static Response *logout(Request *);
static Response *signup(Request *);
static Response *about(Request *);
static Response *notFound(Request *);

int main(int argc, char *argv[])
{
    if (signal(SIGINT,  sig) == SIG_ERR ||
        signal(SIGTERM, sig) == SIG_ERR) {
        fprintf(stderr, "error: failed to bind signal handler\n");
        return 1;
    }

    srand(time(NULL));

    initDB();

    uint16_t server_port = 8080;
    if (argc > 1) {
        if (sscanf(argv[1], "%u", &server_port) == 0 || server_port > 65535) {
            fprintf(stderr, "error: invalid command line argument, using default port 8080.\n");
            server_port = 8080;
        }
    }
    Server *server = serverNew(server_port);
    serverAddHandler(server, notFound);
    serverAddStaticHandler(server);
    serverAddHandler(server, about);
    serverAddHandler(server, signup);
    serverAddHandler(server, logout);
    serverAddHandler(server, login);
    serverAddHandler(server, search);
    serverAddHandler(server, connect);
    serverAddHandler(server, like);
    serverAddHandler(server, unlike);
    serverAddHandler(server, post);
    serverAddHandler(server, profile);
    serverAddHandler(server, dashboard);
    serverAddHandler(server, home);
    serverAddHandler(server, session);
    serverServe(server);

    return 0;
}

/* handlers */

#define invalid(k, v) {          \
    templateSet(template, k, v); \
    valid = false;               \
}

static Response *session(Request *req)
{
    char *sid = kvFindList(req->cookies, "sid");

    if (sid)
        req->account = accountGetBySId(DB, sid);

    return NULL;
}

static Response *home(Request *req)
{
    EXACT_ROUTE(req, "/");

    if (req->account)
        return responseNewRedirect("/dashboard/");

    Response *response = responseNew();
    Template *template = templateNew("templates/index.html");
    responseSetStatus(response, OK);
    templateSet(template, "active", "home");
    templateSet(template, "subtitle", "Home");
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *dashboard(Request *req)
{
    EXACT_ROUTE(req, "/dashboard/");

    if (!req->account)
        return responseNewRedirect("/login/");

    Response *response = responseNew();
    Template *template = templateNew("templates/dashboard.html");

    char *res = NULL;
    char sbuff[128];
    char *bbuff = NULL;
    time_t t;
    bool liked;
    struct tm *info;

    Account *account = NULL;
    Post *post = NULL;
    ListCell *postPCell = NULL;
    ListCell *postCell  = postGetLatestGraph(DB, req->account->id, 0);

    if (postCell)
        res = bsNew("<ul class=\"posts\">");

    while (postCell) {
        post = (Post *)postCell->value;
        account = accountGetById(DB, post->authorId);
        liked = likeLiked(DB, req->account->id, post->id);

        bbuff = bsNewLen("", strlen(post->body) + 256);
        sprintf(bbuff,
                "<li class=\"post-item\">"
                "<div class=\"post-author\">%s</div>"
                "<div class=\"post-content\">"
                "%s"
                "</div>",
                account->name,
                post->body);
        accountDel(account);
        bsLCat(&res, bbuff);

        if (liked) {
            sprintf(sbuff, "<a class=\"btn\" href=\"/unlike/%d/\">Liked</a> - ", post->id);
	    bsLCat(&res, sbuff);
        } else {
            sprintf(sbuff, "<a class=\"btn\" href=\"/like/%d/\">Like</a> - ", post->id);
            bsLCat(&res, sbuff);
        }

        t = post->createdAt;
        info = gmtime(&t);
        info->tm_hour = info->tm_hour + 8;
        strftime(sbuff, 128, "%c GMT+8", info);
        bsLCat(&res, sbuff);
        bsLCat(&res, "</li>");

        bsDel(bbuff);
        postDel(post);
        postPCell = postCell;
        postCell  = postCell->next;

        free(postPCell);
    }

    if (res) {
        bsLCat(&res, "</ul>");
        templateSet(template, "graph", res);
        bsDel(res);
    } else {
        templateSet(template, "graph",
                    "<ul class=\"posts\"><div class=\"not-found\">Nothing here.</div></ul>");
    }

    templateSet(template, "active", "dashboard");
    templateSet(template, "loggedIn", "t");
    templateSet(template, "subtitle", "Dashboard");
    templateSet(template, "accountName", req->account->name);
    responseSetStatus(response, OK);
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *profile(Request *req)
{
    ROUTE(req, "/profile/");

    if (!req->account) return NULL;

    int   id = -1;
    int   idStart = strchr(req->uri + 1, '/') + 1 - req->uri;
    char *idStr = bsSubstr(req->uri, idStart, -1);

    sscanf(idStr, "%d", &id);

    Account *account = accountGetById(DB, id);

    if (!account) return NULL;

    if (account->id == req->account->id)
        return responseNewRedirect("/dashboard/");

    Response *response = responseNew();
    Template *template = templateNew("templates/profile.html");
    Connection *connection = connectionGetByAccountIds(DB,
                                                       req->account->id,
                                                       account->id);
    char connectStr[512];

    if (connection) {
        sprintf(connectStr, "You and %s are connected!", account->name);
    } else {
        sprintf(connectStr,
                "You and %s are not connected."
                " <a href=\"/connect/%d/\">Click here</a> to connect!",
                account->name,
                account->id);
    }

    char *res = NULL;
    char sbuff[128];
    char *bbuff = NULL;
    time_t t;
    bool liked;

    Post *post = NULL;
    ListCell *postPCell = NULL;
    ListCell *postCell  = postGetLatest(DB, account->id, 0);

    if (postCell)
        res = bsNew("<ul class=\"posts\">");

    while (postCell) {
        post = (Post *)postCell->value;
        liked = likeLiked(DB, req->account->id, post->id);

        bbuff = bsNewLen("", strlen(post->body) + 256);
        sprintf(bbuff, "<li class=\"post-item\"><div class=\"post-author\">%s</div>", post->body);
        bsLCat(&res, bbuff);

        if (liked) {
            bsLCat(&res, "Liked - ");
        } else {
            sprintf(sbuff, "<a class=\"btn\" href=\"/like/%d/\">Like</a> - ", post->id);
            bsLCat(&res, sbuff);
        }

        t = post->createdAt;
        strftime(sbuff, 128, "%c GMT", gmtime(&t));
        bsLCat(&res, sbuff);
        bsLCat(&res, "</li>");

        bsDel(bbuff);
        postDel(post);
        postPCell = postCell;
        postCell  = postCell->next;

        free(postPCell);
    }

    if (res) {
        bsLCat(&res, "</ul>");
        templateSet(template, "profilePosts", res);
        bsDel(res);
    } else {
        templateSet(template, "profilePosts",
                    "<h4 class=\"not-found\">This person has not posted "
                    "anything yet!</h4>");
    }

    templateSet(template, "active", "profile");
    templateSet(template, "loggedIn", "t");
    templateSet(template, "subtitle", account->name);
    templateSet(template, "profileId", idStr);
    templateSet(template, "profileName", account->name);
    templateSet(template, "profileEmail", account->email);
    templateSet(template, "profileConnect", connectStr);
    templateSet(template, "accountName", req->account->name);
    responseSetStatus(response, OK);
    responseSetBody(response, templateRender(template));
    connectionDel(connection);
    accountDel(account);
    bsDel(idStr);
    templateDel(template);
    return response;
}

static Response *post(Request *req)
{
    EXACT_ROUTE(req, "/post/");

    if (req->method != POST) return NULL;

    char *postStr = kvFindList(req->postBody, "post");

    if (bsGetLen(postStr) == 0)
        return responseNewRedirect("/dashboard/");
    else if (bsGetLen(postStr) < MAX_BODY_LEN)
        postDel(postCreate(DB, req->account->id, postStr));

    return responseNewRedirect("/dashboard/");
}

static Response *unlike(Request *req)
{
    ROUTE(req, "/unlike/");

    if (!req->account) return NULL;

    int   id = -1;
    int   idStart = strchr(req->uri + 1, '/') + 1 - req->uri;
    char *idStr = bsSubstr(req->uri, idStart, -1);

    sscanf(idStr, "%d", &id);

    Post *post = postGetById(DB, id);
    if (!post) goto fail;

    likeDel(likeDelete(DB, req->account->id, post->authorId, post->id));

    if (kvFindList(req->queryString, "r")) {
        char sbuff[1024];
        sprintf(sbuff, "/profile/%d/", post->authorId);
        bsDel(idStr);
        return responseNewRedirect(sbuff);
    }

fail:
    bsDel(idStr);
    return responseNewRedirect("/dashboard/");
}

static Response *like(Request *req)
{
    ROUTE(req, "/like/");

    if (!req->account) return NULL;

    int   id = -1;
    int   idStart = strchr(req->uri + 1, '/') + 1 - req->uri;
    char *idStr = bsSubstr(req->uri, idStart, -1);

    sscanf(idStr, "%d", &id);

    Post *post = postGetById(DB, id);
    if (!post) goto fail;

    likeDel(likeCreate(DB, req->account->id, post->authorId, post->id));

    if (kvFindList(req->queryString, "r")) {
        char sbuff[1024];
        sprintf(sbuff, "/profile/%d/", post->authorId);
        bsDel(idStr);
        return responseNewRedirect(sbuff);
    }

fail:
    bsDel(idStr);
    return responseNewRedirect("/dashboard/");
}

static Response *connect(Request *req)
{
    ROUTE(req, "/connect/");
    if (!req->account) return NULL;

    int   id = -1;
    int   idStart = strchr(req->uri + 1, '/') + 1 - req->uri;
    char *idStr = bsSubstr(req->uri, idStart, -1);

    sscanf(idStr, "%d", &id);

    Account *account = accountGetById(DB, id);
    if (!account) goto fail;

    connectionDel(connectionCreate(DB, req->account->id, account->id));

    char sbuff[1024];
    sprintf(sbuff, "/profile/%d/", account->id);
    bsDel(idStr);
    return responseNewRedirect(sbuff);

fail:
    bsDel(idStr);
    return responseNewRedirect("/dashboard/");
}

static Response *search(Request *req)
{
    EXACT_ROUTE(req, "/search/");

    if (!req->account)
        return responseNewRedirect("/login/");

    char *query = kvFindList(req->queryString, "q");

    if (!query) return NULL;

    char *res = NULL;
    char  sbuff[1024];

    Account *account = NULL;
    ListCell *accountPCell = NULL;
    ListCell *accountCell  = accountSearch(DB, query, 0);

    if (accountCell)
        res = bsNew("<ul class=\"search-results\">");

    while (accountCell) {
        account = (Account *)accountCell->value;

        sprintf(sbuff,
                "<li><a href=\"/profile/%d/\">%s</a> (<span>%s</span>)</li>\n",
                account->id, account->name, account->email);
        bsLCat(&res, sbuff);

        accountDel(account);
        accountPCell = accountCell;
        accountCell  = accountCell->next;

        free(accountPCell);
    }

    if (res)
        bsLCat(&res, "</ul>");

    Response *response = responseNew();
    Template *template = templateNew("templates/search.html");
    responseSetStatus(response, OK);

    if (!res) {
        templateSet(template, "results",
                    "<h4 class=\"not-found\">There were no results "
                    "for your query.</h4>");
    } else {
        templateSet(template, "results", res);
        bsDel(res);
    }

    templateSet(template, "searchQuery", query);
    templateSet(template, "active", "search");
    templateSet(template, "loggedIn", "t");
    templateSet(template, "subtitle", "Search");
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *login(Request *req)
{
    EXACT_ROUTE(req, "/login/");

    if (req->account)
        return responseNewRedirect("/dashboard/");

    Response *response = responseNew();
    Template *template = templateNew("templates/login.html");
    responseSetStatus(response, OK);
    templateSet(template, "active", "login");
    templateSet(template, "subtitle", "Login");

    if (req->method == POST) {
        bool valid = true;

        char *username = kvFindList(req->postBody, "username");
        char *password = kvFindList(req->postBody, "password");

        if (!username) {
            invalid("usernameError", "Username missing!");
        } else {
            templateSet(template, "formUsername", username);
        }

        if (!password) {
            invalid("passwordError", "Password missing!");
        }

        if (valid) {
            Session *session = sessionCreate(DB, username, password);
            if (session) {
                responseSetStatus(response, FOUND);
                responseAddCookie(response, "sid", session->sessionId,
                                  NULL, NULL, 3600 * 24 * 30);
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

static Response *logout(Request *req)
{
    EXACT_ROUTE(req, "/logout/");
    if (!req->account)
        return responseNewRedirect("/");

    Response *response = responseNewRedirect("/");
    responseAddCookie(response, "sid", "", NULL, NULL, -1);
    return response;
}

static Response *signup(Request *req)
{
    EXACT_ROUTE(req, "/signup/");

    if (req->account)
        return responseNewRedirect("/dashboard/");

    Response *response = responseNew();
    Template *template = templateNew("templates/signup.html");
    templateSet(template, "active", "signup");
    templateSet(template, "subtitle", "Sign Up");
    responseSetStatus(response, OK);

    if (req->method == POST) {
        bool valid = true;
        char *name = kvFindList(req->postBody, "name");
        char *email = kvFindList(req->postBody, "email");
        char *username = kvFindList(req->postBody, "username");
        char *password = kvFindList(req->postBody, "password");
        char *confirmPassword = kvFindList(req->postBody, "confirm-password");

        if (!name) {
            invalid("nameError", "You must enter your name!");
        } else if (strlen(name) < 5 || strlen(name) > 50) {
            invalid("nameError",
                    "Your name must be between 5 and 50 characters long.");
        } else {
            templateSet(template, "formName", name);
        }

        if (!email) {
            invalid("emailError", "You must enter an email!");
        } else if (strchr(email, '@') == NULL) {
            invalid("emailError", "Invalid email.");
        } else if (strlen(email) < 3 || strlen(email) > 50) {
            invalid("emailError",
                    "Your email must be between 3 and 50 characters long.");
        } else if (!accountCheckEmail(DB, email)) {
            invalid("emailError", "This email is taken.");
        } else {
            templateSet(template, "formEmail", email);
        }

        if (!username) {
            invalid("usernameError", "You must enter a username!");
        } else if (strlen(username) < 3 || strlen(username) > 50) {
            invalid("usernameError",
                    "Your username must be between 3 and 50 characters long.");
        } else if (!accountCheckUsername(DB, username)) {
            invalid("usernameError", "This username is taken.");
        } else {
            templateSet(template, "formUsername", username);
        }

        if (!password) {
            invalid("passwordError", "You must enter a password!");
        } else if (strlen(password) < 8) {
            invalid("passwordError",
                    "Your password must be at least 8 characters long!");
        }

        if (!confirmPassword) {
            invalid("confirmPasswordError", "You must confirm your password.");
        } else if (strcmp(password, confirmPassword) != 0) {
            invalid("confirmPasswordError",
                    "The two passwords must be the same.");
        }

        if (valid) {
            Account *account = accountCreate(DB, name,
                                             email, username, password);

            if (account) {
                responseSetStatus(response, FOUND);
                responseAddHeader(response, "Location", "/login/");
                templateDel(template);
                accountDel(account);
                return response;
            } else {
                invalid("nameError",
                        "Unexpected error. Please try again later.");
            }
        }
    }

    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *about(Request *req)
{
    EXACT_ROUTE(req, "/about/");

    Response *response = responseNew();
    Template *template = templateNew("templates/about.html");
    templateSet(template, "active", "about");
    templateSet(template, "loggedIn", req->account ? "t" : "");
    templateSet(template, "subtitle", "About");
    responseSetStatus(response, OK);
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

static Response *notFound(Request *req)
{
    Response *response = responseNew();
    Template *template = templateNew("templates/404.html");
    templateSet(template, "loggedIn", req->account ? "t" : "");
    templateSet(template, "subtitle", "404 Not Found");
    responseSetStatus(response, NOT_FOUND);
    responseSetBody(response, templateRender(template));
    templateDel(template);
    return response;
}

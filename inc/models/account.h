#ifndef MODELS_ACCOUNT_H
#define MODELS_ACCOUNT_H

#include <sqlite3.h>
#include <stdbool.h>

typedef struct Account {
    int id;
    int createdAt;

    char *name;
    char *email;
    char *username;
} Account;

Account *accountNew(int, int, char *, char *, char *);
Account *accountCreate(sqlite3 *, char *, char *, char *, char *);
Account *accountGetByEmail(sqlite3 *, char *);
Account *accountGetBySId(sqlite3 *, char *);
bool     accountCheckUsername(sqlite3 *, char *);
bool     accountCheckEmail(sqlite3 *, char *);
void     accountDel(Account *);

#endif

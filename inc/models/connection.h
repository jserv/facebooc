#ifndef MODELS_CONNECTION_H
#define MODELS_CONNECTION_H

#include <sqlite3.h>

typedef struct Connection {
    int id;
    int createdAt;
    int account1;
    int account2;
} Connection;

Connection *connectionNew(int, int, int, int);
Connection *connectionCreate(sqlite3 *, int, int);
void        connectionDel(Connection *);

#endif

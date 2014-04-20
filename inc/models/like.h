#ifndef MODELS_LIKE_H
#define MODELS_LIKE_H

#include <sqlite3.h>

typedef struct Like {
    int id;
    int createdAt;
    int account;
    int author;
    int post;
} Like;

Like *likeNew(int, int, int, int, int);
Like *likeCreate(sqlite3 *, int, int, int);
void  likeDel(Like *);

#endif

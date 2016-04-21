#ifndef MODELS_POST_H
#define MODELS_POST_H

#include <sqlite3.h>

#include "list.h"

#define MAX_BODY_LEN 65536

typedef struct Post {
    int id;
    int createdAt;
    int authorId;

    char *body;
} Post;

Post     *postNew(int, int, int, char *);
Post     *postCreate(sqlite3 *, int, char *);
Post     *postGetById(sqlite3 *, int);
ListCell *postGetLatest(sqlite3 *, int, int);
ListCell *postGetLatestGraph(sqlite3 *, int, int);
void      postDel(Post *);

#endif

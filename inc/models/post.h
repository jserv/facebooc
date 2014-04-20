#ifndef MODELS_POST_H
#define MODELS_POST_H

#include <sqlite3.h>

typedef struct Post {
    int id;
    int createdAt;
    int author;

    char *body;
} Post;

Post *postNew(int, int, int, char *);
Post *postCreate(sqlite3 *, int, char *);
void  postDel(Post *);

#endif

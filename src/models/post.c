#include <time.h>

#include "bs.h"
#include "models/post.h"

Post *postNew(int id, int createdAt, int authorId, char *body)
{
    Post *post = malloc(sizeof(Post));

    post->id = id;
    post->createdAt = createdAt;
    post->authorId  = authorId;
    post->body = bsNew(body);

    return post;
}

Post *postCreate(sqlite3 *DB, int authorId, char *body)
{
    int rc, t;
    Post *post = NULL;
    sqlite3_stmt *statement;

    t  = time(NULL);
    rc = sqlite3_prepare_v2(
             DB,
             "INSERT INTO posts(createdAt, author, body)"
             "     VALUES      (        ?,      ?,    ?)",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;

    char *escapedBody = bsEscape(body);

    if (sqlite3_bind_int(statement, 1, t) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 2, authorId) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 3, escapedBody, -1, NULL) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(statement) == SQLITE_DONE)
        post = postNew(sqlite3_last_insert_rowid(DB),
                       t, authorId, body);

fail:
    bsDel(escapedBody);
    sqlite3_finalize(statement);
    return post;
}

Post *postGetById(sqlite3 *DB, int id)
{
    if (id == -1)
        return NULL;

    Post *post = NULL;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, author, body"
                           "  FROM posts"
                           " WHERE id = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_int(statement, 1, id) != SQLITE_OK)  goto fail;
    if (sqlite3_step(statement)            != SQLITE_ROW) goto fail;

    post = postNew(sqlite3_column_int(statement, 0),
                   sqlite3_column_int(statement, 1),
                   sqlite3_column_int(statement, 2),
                   bsNewline2BR((char *)sqlite3_column_text(statement, 3)));

fail:
    sqlite3_finalize(statement);
    return post;
}

ListCell *postGetLatest(sqlite3 *DB, int accountId, int page)
{
    if (accountId == -1)
        return NULL;

    int rc;
    Post *post = NULL;
    ListCell *posts = NULL;
    sqlite3_stmt *statement;

    rc = sqlite3_prepare_v2(
             DB,
             "SELECT id, createdAt, author, body"
             "  FROM posts"
             " WHERE author = ?"
             " ORDER BY createdAt DESC"
             " LIMIT 10 "
             "OFFSET ?",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if (sqlite3_bind_int(statement, 1, accountId) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 2, page * 10) != SQLITE_OK) goto fail;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        post = postNew(sqlite3_column_int(statement, 0),
                       sqlite3_column_int(statement, 1),
                       sqlite3_column_int(statement, 2),
                       bsNewline2BR((char *)sqlite3_column_text(statement, 3)));
        posts = listCons(post, sizeof(Post), posts);
    }

    posts = listReverse(posts);

fail:
    sqlite3_finalize(statement);
    return posts;
}

ListCell *postGetLatestGraph(sqlite3 *DB, int accountId, int page)
{
    if (accountId == -1)
        return NULL;

    int rc;
    Post *post = NULL;
    ListCell *posts = NULL;
    sqlite3_stmt *statement;

    rc = sqlite3_prepare_v2(
             DB,
             "SELECT posts.id, posts.createdAt, posts.author, posts.body"
             "  FROM posts"
             "  LEFT OUTER JOIN connections"
             "    ON posts.author = connections.account2"
             " WHERE connections.account1 = ?"
             "    OR posts.author = ?"
             " ORDER BY posts.createdAt DESC"
             " LIMIT 10 "
             "OFFSET ?",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if (sqlite3_bind_int(statement, 1, accountId) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 2, accountId) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 3, page * 10) != SQLITE_OK) goto fail;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        post = postNew(sqlite3_column_int(statement, 0),
                       sqlite3_column_int(statement, 1),
                       sqlite3_column_int(statement, 2),
                       bsNewline2BR((char *)sqlite3_column_text(statement, 3)));
        posts = listCons(post, sizeof(Post), posts);
    }

    posts = listReverse(posts);

fail:
    sqlite3_finalize(statement);
    return posts;
}

void postDel(Post *post)
{
    bsDel(post->body);
    free(post);
}

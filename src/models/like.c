#include <stdlib.h>
#include <time.h>

#include "models/like.h"

Like *likeNew(int id, int createdAt, int accountId, int authorId, int postId)
{
    Like *like = malloc(sizeof(Like));

    like->id        = id;
    like->createdAt = createdAt;
    like->accountId = accountId;
    like->authorId  = authorId;
    like->postId    = postId;

    return like;
}

Like *likeCreate(sqlite3 *DB, int accountId, int authorId, int postId)
{
    int rc, t;
    Like *like = NULL;
    sqlite3_stmt *statement;

    t  = time(NULL);
    rc = sqlite3_prepare_v2(
             DB,
             "INSERT INTO likes(createdAt, account, author, post)"
             "     VALUES      (        ?,       ?,      ?,    ?)",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if ((sqlite3_bind_int(statement, 1, t) != SQLITE_OK) ||
        (sqlite3_bind_int(statement, 2, accountId) != SQLITE_OK) ||
        (sqlite3_bind_int(statement, 3, authorId) != SQLITE_OK) ||
        (sqlite3_bind_int(statement, 4, postId) != SQLITE_OK))
	goto fail;

    if (sqlite3_step(statement) == SQLITE_DONE)
        like = likeNew(sqlite3_last_insert_rowid(DB),
                       t, accountId, authorId, postId);

fail:
    sqlite3_finalize(statement);
    return like;
}

Like *likeDelete(sqlite3 *DB, int accountId, int authorId, int postId)
{
    int rc, t;
    Like *like = NULL;
    sqlite3_stmt *statement;

    t  = time(NULL);
    rc = sqlite3_prepare_v2(DB,
			    "DELETE"
			    " FROM likes"
			    " WHERE account = ?"
			    "   AND post = ?",
			    -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if ((sqlite3_bind_int(statement, 1, accountId) != SQLITE_OK) ||
        (sqlite3_bind_int(statement, 2, postId) != SQLITE_OK))
	goto fail;

    if (sqlite3_step(statement) == SQLITE_DONE)
        like = likeNew(sqlite3_last_insert_rowid(DB),
                       t, accountId, authorId, postId);

fail:
    sqlite3_finalize(statement);
    return like;
}

bool likeLiked(sqlite3 *DB, int accountId, int postId)
{
    int rc;
    bool res = false;
    sqlite3_stmt *statement;

    rc = sqlite3_prepare_v2(DB,
                            "SELECT id"
                            "  FROM likes"
                            " WHERE account = ?"
                            "   AND post = ?",
                            -1, &statement, NULL);

    if (rc != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, 1, accountId) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 2, postId) != SQLITE_OK) goto fail;

    res = sqlite3_step(statement) == SQLITE_ROW;

fail:
    sqlite3_finalize(statement);
    return res;
}

void likeDel(Like *like)
{
    free(like);
}

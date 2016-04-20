#include <time.h>

#include "bs.h"
#include "models/session.h"

Session *sessionNew(int id, int createdAt, int accountId, char *sessionId)
{
    Session *session = malloc(sizeof(Session));

    session->id = id;
    session->createdAt = createdAt;
    session->accountId = accountId;
    session->sessionId = bsNew(sessionId);

    return session;
}

Session *sessionGetBySId(sqlite3 *DB, char *sid)
{
    Session *session = NULL;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, account, session"
                           "  FROM sessions"
                           " WHERE session = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_text(statement, 1, sid, -1, NULL) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(statement) == SQLITE_ROW) {
        session = sessionNew(sqlite3_column_int(statement, 0),
                             sqlite3_column_int(statement, 1),
                             sqlite3_column_int(statement, 2),
                             (char *)sqlite3_column_text(statement, 3));
    }

fail:
    sqlite3_finalize(statement);
    return session;
}

Session *sessionCreate(sqlite3 *DB, char *username, char *password)
{
    int aid;
    char *sid = NULL;
    Session *session = NULL;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id"
                           "  FROM accounts"
                           " WHERE username = ?"
                           "   AND password = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_text(statement, 1, username, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_text(statement, 2, password, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(statement) != SQLITE_ROW)
        goto fail;

    sid = bsRandom(24, username);
    aid = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    if (sqlite3_prepare_v2(DB,
                           "INSERT INTO sessions(createdAt, account, session)"
                           "     VALUES         (        ?,       ?,       ?)",
                           -1, &statement, NULL) != SQLITE_OK) {
        goto fail;
    }

    if (sqlite3_bind_int(statement, 1, time(NULL)) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 2, aid) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 3, sid, -1, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_step(statement) != SQLITE_DONE)
        goto fail;

    session = sessionGetBySId(DB, sid);

fail:
    if (sid)
        bsDel(sid);

    sqlite3_finalize(statement);

    return session;
}

void sessionDel(Session *session)
{
    bsDel(session->sessionId);
    free(session);
}

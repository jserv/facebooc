#include <stdio.h>
#include <time.h>

#include "bs.h"
#include "models/account.h"
#include "models/session.h"

Account *accountNew(int id, int createdAt, char *name,
                    char *email, char *username)
{
    Account *account = malloc(sizeof(Account));

    account->id        = id;
    account->createdAt = createdAt;
    account->name      = bsNew(name);
    account->email     = bsNew(email);
    account->username  = bsNew(username);

    return account;
}

Account *accountGetById(sqlite3 *DB, int id)
{
    if (id == -1) return NULL;

    Account *account = NULL;
    sqlite3_stmt *statement;
    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, name, email, username"
                           "  FROM accounts"
                           " WHERE id = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_int(statement, 1, id) != SQLITE_OK)  goto fail;
    if (sqlite3_step(statement)            != SQLITE_ROW) goto fail;

    account = accountNew(sqlite3_column_int(statement, 0),
                         sqlite3_column_int(statement, 1),
                         (char *)sqlite3_column_text(statement, 2),
                         (char *)sqlite3_column_text(statement, 3),
                         (char *)sqlite3_column_text(statement, 4));

fail:
    sqlite3_finalize(statement);
    return account;
}

Account *accountCreate(sqlite3 *DB, char *name,
                       char *email, char *username, char *password)
{
    int rc;
    Account *account = NULL;
    sqlite3_stmt *statement;
    rc = sqlite3_prepare_v2(
             DB,
             "INSERT INTO accounts(createdAt, name, email, username, password)"
             "     VALUES         (        ?,    ?,     ?,        ?,        ?)",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;

    char *escapedName  = bsEscape(name);
    char *escapedEmail = bsEscape(email);

    if (sqlite3_bind_int(statement, 1, time(NULL)) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 2, escapedName, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_text(statement, 3, escapedEmail, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_text(statement, 4, username, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_text(statement, 5, password, -1, NULL) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(statement) == SQLITE_DONE)
        account = accountGetByEmail(DB, email);

fail:
    bsDel(escapedName);
    bsDel(escapedEmail);
    sqlite3_finalize(statement);
    return account;
}

Account *accountGetByEmail(sqlite3 *DB, char *email)
{
    if (!email) return NULL;

    Account *account = NULL;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, name, email, username"
                           "  FROM accounts"
                           " WHERE email = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_text(statement, 1, email, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(statement) != SQLITE_ROW) goto fail;

    account = accountNew(sqlite3_column_int(statement, 0),
                         sqlite3_column_int(statement, 1),
                         (char *)sqlite3_column_text(statement, 2),
                         (char *)sqlite3_column_text(statement, 3),
                         (char *)sqlite3_column_text(statement, 4));

fail:
    sqlite3_finalize(statement);
    return account;
}

Account *accountGetBySId(sqlite3 *DB, char *sid)
{
    if (!sid) return NULL;

    Session *session = sessionGetBySId(DB, sid);
    if (!session) return NULL;

    Account *account = NULL;
    sqlite3_stmt *statement;
    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, name, email, username"
                           "  FROM accounts"
                           " WHERE id = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_int(statement, 1, session->accountId) != SQLITE_OK) goto fail;
    if (sqlite3_step(statement) != SQLITE_ROW)                           goto fail;

    account = accountNew(sqlite3_column_int(statement, 0),
                         sqlite3_column_int(statement, 1),
                         (char *)sqlite3_column_text(statement, 2),
                         (char *)sqlite3_column_text(statement, 3),
                         (char *)sqlite3_column_text(statement, 4));


fail:
    sessionDel(session);
    sqlite3_finalize(statement);
    return account;
}

ListCell *accountSearch(sqlite3 *DB, char *query, int page)
{
    if (!query) return NULL;

    int rc;
    Account *account = NULL;
    ListCell *accounts = NULL;
    sqlite3_stmt *statement;

    rc = sqlite3_prepare_v2(
             DB,
             "SELECT id, createdAt, name, email, username"
             "  FROM accounts"
             " WHERE name     LIKE '%' || ? || '%'"
             "    OR email    LIKE '%' || ? || '%'"
             "    OR username LIKE '%' || ? || '%'"
             " ORDER BY createdAt DESC"
             " LIMIT 10 "
             "OFFSET ?",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if (sqlite3_bind_text(statement, 1, query, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_text(statement, 2, query, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_text(statement, 3, query, -1, NULL) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_int(statement, 4, page * 10) != SQLITE_OK)
        goto fail;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        account = accountNew(sqlite3_column_int(statement, 0),
                             sqlite3_column_int(statement, 1),
                             (char *)sqlite3_column_text(statement, 2),
                             (char *)sqlite3_column_text(statement, 3),
                             (char *)sqlite3_column_text(statement, 4));
        accounts = listCons(account, sizeof(Account), accounts);
    }

fail:
    sqlite3_finalize(statement);
    return accounts;
}

bool accountCheckUsername(sqlite3 *DB, char *username)
{
    bool res;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id FROM accounts WHERE username = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_bind_text(statement, 1, username, -1, NULL) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    res = sqlite3_step(statement) != SQLITE_ROW;

    sqlite3_finalize(statement);

    return res;
}

bool accountCheckEmail(sqlite3 *DB, char *email)
{
    bool res;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id FROM accounts WHERE email = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_bind_text(statement, 1, email, -1, NULL) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }
    res = sqlite3_step(statement) != SQLITE_ROW;

    sqlite3_finalize(statement);

    return res;
}

void accountDel(Account *account)
{
    bsDel(account->name);
    bsDel(account->email);
    bsDel(account->username);
    free(account);
}

#include <time.h>

#include "bs.h"
#include "models/account.h"

Account *accountNew(int id, int createdAt, char *name, char *email, char *username) {
    Account *account = malloc(sizeof(Account));

    account->id        = id;
    account->createdAt = createdAt;
    account->name      = bsNew(name);
    account->email     = bsNew(email);
    account->username  = bsNew(username);

    return account;
}

Account *accountCreate(sqlite3 *DB, char *name, char *email, char *username, char *password) {
    int rc;
    Account *account = NULL;
    sqlite3_stmt *statement;

    rc = sqlite3_prepare_v2(
        DB,
        "INSERT INTO accounts(createdAt, name, email, username, password)"
        "     VALUES         (        ?,    ?,     ?,        ?,        ?)",
        -1, &statement, NULL);

    if (rc != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, 1, time(NULL))          != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 2, name, -1, NULL)     != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 3, email, -1, NULL)    != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 4, username, -1, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(statement, 5, password, -1, NULL) != SQLITE_OK) goto fail;

    if (sqlite3_step(statement) == SQLITE_DONE) {
        account = accountGetByEmail(DB, email);
    }

    sqlite3_finalize(statement);

    return account;

fail:
    sqlite3_finalize(statement);
    return NULL;
}

Account *accountGetByEmail(sqlite3 *DB, char *email) {
    if (email == NULL)
        return NULL;

    Account *account;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, name, username, email"
                           "  FROM accounts"
                           " WHERE email = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_text(statement, 1, email, -1, NULL) != SQLITE_OK)  goto fail;
    if (sqlite3_step(statement)                          != SQLITE_ROW) goto fail;

    account = accountNew(sqlite3_column_int(statement, 0),
                         sqlite3_column_int(statement, 1),
                         (char *)sqlite3_column_text(statement, 2),
                         (char *)sqlite3_column_text(statement, 3),
                         (char *)sqlite3_column_text(statement, 4));

    sqlite3_finalize(statement);

    return account;

fail:
    sqlite3_finalize(statement);
    return NULL;
}

Account *accountGetBySId(sqlite3 *DB, char *sid) {
    if (sid == NULL)
        return NULL;

    int aid;
    Account *account;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB, "SELECT account FROM sessions WHERE sid = ?", -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_text(statement, 1, sid, -1, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_step(statement) != SQLITE_ROW)                       goto fail;

    aid = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    if (sqlite3_prepare_v2(DB,
                           "SELECT id, createdAt, name, username, email"
                           "  FROM accounts"
                           " WHERE id = ?",
                           -1, &statement, NULL) != SQLITE_OK) {
        return NULL;
    }

    if (sqlite3_bind_int(statement, 1, aid) != SQLITE_OK) goto fail;
    if (sqlite3_step(statement) != SQLITE_ROW)            goto fail;

    account = accountNew(sqlite3_column_int(statement, 0),
                         sqlite3_column_int(statement, 1),
                         (char *)sqlite3_column_text(statement, 2),
                         (char *)sqlite3_column_text(statement, 3),
                         (char *)sqlite3_column_text(statement, 4));

    sqlite3_finalize(statement);

    return account;

fail:
    sqlite3_finalize(statement);
    return NULL;
}

bool accountCheckUsername(sqlite3 *DB, char *username) {
    bool res;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB, "SELECT id FROM accounts WHERE username = ?", -1, &statement, NULL) != SQLITE_OK) {
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

bool accountCheckEmail(sqlite3 *DB, char *email) {
    bool res;
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB, "SELECT id FROM accounts WHERE email = ?", -1, &statement, NULL) != SQLITE_OK) {
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

void accountDel(Account *account) {
    bsDel(account->name);
    bsDel(account->email);
    bsDel(account->username);
    free(account);
}

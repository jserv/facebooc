#include <stdlib.h>
#include <time.h>

#include "models/connection.h"

Connection *connectionNew(int id, int createdAt,
                          int account1Id, int account2Id)
{
    Connection *connection = malloc(sizeof(Connection));

    connection->id = id;
    connection->createdAt = createdAt;
    connection->account1Id = account1Id;
    connection->account2Id = account2Id;

    return connection;
}

Connection *connectionCreate(sqlite3 *DB,
                             int account1Id, int account2Id)
{
    int rc, t;
    Connection *connection = NULL;
    sqlite3_stmt *statement;

    t  = time(NULL);
    rc = sqlite3_prepare_v2(
             DB,
             "INSERT INTO connections(createdAt, account1, account2)"
             "     VALUES            (        ?,        ?,        ?)",
             -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if (sqlite3_bind_int(statement, 1, t) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(statement, 2, account1Id) != SQLITE_OK)
        goto fail;
    if (sqlite3_bind_int(statement, 3, account2Id) != SQLITE_OK) goto fail;

    if (sqlite3_step(statement) == SQLITE_DONE) {
        connection = connectionNew(sqlite3_last_insert_rowid(DB),
                                   t, account1Id, account2Id);
    }

fail:
    sqlite3_finalize(statement);
    return connection;
}

Connection *connectionGetByAccountIds(sqlite3 *DB,
                                      int account1Id, int account2Id)
{
    if (account1Id == -1 || account2Id == -1)
        return NULL;

    int rc;
    Connection *connection = NULL;
    sqlite3_stmt *statement;


    rc = sqlite3_prepare_v2(DB,
                            "SELECT id, createdAt, account1, account2"
                            "  FROM connections"
                            " WHERE account1 = ?"
                            "   AND account2 = ?",
                            -1, &statement, NULL);

    if (rc != SQLITE_OK) return NULL;
    if (sqlite3_bind_int(statement, 1, account1Id) != SQLITE_OK)  goto fail;
    if (sqlite3_bind_int(statement, 2, account2Id) != SQLITE_OK)  goto fail;
    if (sqlite3_step(statement)                    != SQLITE_ROW) goto fail;

    connection = connectionNew(sqlite3_column_int(statement, 0),
                               sqlite3_column_int(statement, 1),
                               sqlite3_column_int(statement, 2),
                               sqlite3_column_int(statement, 3));

fail:
    sqlite3_finalize(statement);
    return connection;
}

void connectionDel(Connection *connection)
{
    free(connection);
}

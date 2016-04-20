#include <string.h>

#include "list.h"

ListCell *listCons(void *value, size_t size, ListCell *next)
{
    ListCell *cell = malloc(sizeof(ListCell));

    cell->next  = next;
    cell->value = malloc(size);
    cell->size  = size;

    memcpy(cell->value, value, size);

    return cell;
}

ListCell *listReverse(ListCell *cell)
{
    ListCell *prev, *next;

    while (cell != NULL) {
        next = cell->next;
        cell->next = prev;
        prev = cell;
        cell = next;
    }

    return prev;
}

void listDel(ListCell *cell)
{
    if (cell == NULL) return;

    ListCell *next;

    do {
        next = cell->next;

        free(cell->value);
        free(cell);

        cell = next;
    } while (cell != NULL);
}

IterationResult listForEach(ListCell *cell, ListIterator iterator)
{
    if (cell == NULL) return DONE;

    bool res;

    do {
        res  = iterator(cell->value);
        cell = cell->next;
    } while (cell != NULL && res);

    return (res) ? DONE : BREAK;
}

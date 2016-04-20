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
    ListCell *prev = NULL;
    while (cell) {
        ListCell *next = cell->next;
        cell->next = prev;
        prev = cell;
        cell = next;
    }

    return prev;
}

void listDel(ListCell *cell)
{
    if (!cell) return;
    do {
        ListCell *next = cell->next;
        free(cell->value);
        free(cell);
        cell = next;
    } while (cell);
}

IterationResult listForEach(ListCell *cell, ListIterator iterator)
{
    if (!cell) return DONE;

    bool res;
    do {
        res  = iterator(cell->value);
        cell = cell->next;
    } while (cell && res);

    return res ? DONE : BREAK;
}

#include <stdio.h>
#include <string.h>

#include "bs.h"
#include "kv.h"

KV *kvNew(char *key, char *value)
{
    KV *kv = malloc(sizeof(KV));

    kv->key = bsNew(key);
    kv->value = bsNew(value);

    return kv;
}

void kvDel(KV *kv)
{
    bsDel(kv->key);
    bsDel(kv->value);
    free(kv);
}

static bool kvDelEach(void *kv)
{
    if (kv) {
        bsDel(((KV *)kv)->key);
        bsDel(((KV *)kv)->value);
    }

    return true;
}

void kvDelList(ListCell *list)
{
    listForEach(list, kvDelEach);
    listDel(list);
}

static bool kvPrintEach(void *kv)
{
    fprintf(stdout, "%s: %s\n",
            ((KV *)kv)->key,
            ((KV *)kv)->value);

    return true;
}

void kvPrintList(ListCell *list)
{
    listForEach(list, kvPrintEach);
}

char *kvFindList(ListCell *cell, char *key)
{
    while (cell) {
        if (!strcmp(((KV *)cell->value)->key, key))
            return ((KV *)cell->value)->value;
        cell = cell->next;
    }

    return NULL;
}

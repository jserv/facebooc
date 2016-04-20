#ifndef KV_H
#define KV_H

#include <stdbool.h>

#include "list.h"

typedef struct KV {
    char *key;
    char *value;
} KV;

KV   *kvNew(char *, char *);
void  kvDel(KV *);
void  kvDelList(ListCell *);
void  kvPrintList(ListCell *);
char *kvFindList(ListCell *, char *);

#endif

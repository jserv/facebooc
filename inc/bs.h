#ifndef BS_H
#define BS_H

#include <stdint.h>
#include <stdlib.h>

#include "list.h"

#define BS_HEADER_LEN 4

char     *bsNew(const char *);
char     *bsNewLen(char *, size_t);
char     *bsCat(char *, char *);
char     *bsSubstr(char *, uint32_t, int32_t);
char     *bsRandom(uint32_t, char *);
char     *bsEscape(char *);
void      bsLCat(char **, char *);
void      bsDel(char *);
void      bsSetLen(char *, uint32_t);
uint32_t  bsGetLen(char *);

#endif

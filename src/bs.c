#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "bs.h"

char *bsNew(const char *str) {
    size_t len = strlen(str);

    char *bs = BS_HEADER_LEN +
        malloc(sizeof(char) * BS_HEADER_LEN +
               sizeof(char) * len + 1);

    strcpy(bs, str);

    bsSetLen(bs, len);

    return bs;
}

char *bsNewLen(char *buf, size_t len) {
    char *bs = BS_HEADER_LEN +
        malloc(sizeof(char) * BS_HEADER_LEN +
               sizeof(char) * len + 1);

    memcpy(bs, buf, len);

    bsSetLen(bs, len);

    return bs;
}

char *bsCat(char *bs1, char *bs2) {
    size_t len1 = bsGetLen(bs1);
    size_t len2 = bsGetLen(bs2);
    size_t len  = len1 + len2;

    char *bs = BS_HEADER_LEN +
        malloc(sizeof(char) * BS_HEADER_LEN +
               sizeof(char) * len + 1);

    strcpy(bs, bs1);
    strcpy(bs + len1, bs2);

    bsSetLen(bs, len);

    return bs;
}

void bsLCat(char **orig, char *s) {
    size_t lenO = bsGetLen(*orig);
    size_t lenS = strlen(s);
    size_t len  = lenO + lenS;

    *orig = BS_HEADER_LEN +
        (char *)realloc(
            *orig - BS_HEADER_LEN,
            sizeof(char) * BS_HEADER_LEN +
            sizeof(char) * len + 1);

    strcpy(*orig + lenO, s);

    bsSetLen(*orig, len);
}

char *bsSubstr(char *orig, uint32_t beginning, int32_t end) {
    size_t len    = bsGetLen(orig);
    size_t newLen = (end <= 0)
        ? len - beginning + end
        : end - beginning;

    assert(newLen >  0);
    assert(newLen <= len);

    char *bs = BS_HEADER_LEN +
        malloc(sizeof(char) * BS_HEADER_LEN +
               sizeof(char) * newLen + 1);

    memcpy(bs, orig + beginning, newLen);

    bsSetLen(bs, newLen);

    return bs;
}

void bsDel(char *bs) {
    free(bs - BS_HEADER_LEN);
}

void bsSetLen(char *bs, uint32_t len) {
    *(bs + 0 - BS_HEADER_LEN) = len >> 24 & 0xFF;
    *(bs + 1 - BS_HEADER_LEN) = len >> 16 & 0xFF;
    *(bs + 2 - BS_HEADER_LEN) = len >>  8 & 0xFF;
    *(bs + 3 - BS_HEADER_LEN) = len       & 0xFF;
    *(bs + len + 1) = '\0';
}

uint32_t bsGetLen(char *bs) {
    return
        (((char)*(bs + 0 - BS_HEADER_LEN) & 0xFF) << 24) |
        (((char)*(bs + 1 - BS_HEADER_LEN) & 0xFF) << 16) |
        (((char)*(bs + 2 - BS_HEADER_LEN) & 0xFF) <<  8) |
        (((char)*(bs + 3 - BS_HEADER_LEN) & 0xFF)      );
}

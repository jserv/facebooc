#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "list.h"

typedef struct Template {
    char *filename;

    ListCell *context;
} Template;

Template *templateNew(char *);
void      templateDel(Template *);
void      templateSet(Template *, char *, char *);
char     *templateRender(Template *);

#endif

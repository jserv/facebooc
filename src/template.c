#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bs.h"
#include "kv.h"
#include "template.h"

Template *templateNew(char *filename)
{
    Template *template = malloc(sizeof(Template));

    template->filename = filename;
    template->context = NULL;

    return template;
}

void templateDel(Template *template)
{
    if (template->context)
        kvDelList(template->context);

    free(template);
}

void templateSet(Template *template, char *key, char *value)
{
    template->context =
        listCons(kvNew(key, value), sizeof(KV), template->context);
}

char *templateRender(Template *template)
{
    FILE *file = fopen(template->filename, "r");

    if (!file) {
        fprintf(stderr, "error: template '%s' not found\n", template->filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    rewind(file);

    // For cross platform or we can use mmap
    char *buff = malloc(sizeof(char) * (len + 2));
    (void) !fread(buff + 1, sizeof(char), len, file);
    fclose(file);

    buff[0] = ' ';
    buff[len + 1] = '\0';

    char *res = bsNew("");
    char *pos = NULL;
    // VARIABLES
    char *segment = strtok_r(buff, "{\0", &pos);
    assert(segment);
    bsLCat(&res, segment + 1);

    bool rep = false;
    for (;;) {
        segment = strtok_r(NULL, "}\0", &pos);

        if (!segment)
            break;

        if (*segment == '{') {
            rep = true;
            segment += 1;
            char *val = kvFindList(template->context, segment);
            if (val)
                bsLCat(&res, val);
        } else if (*segment == '%') {
            rep = true;
            segment += 1;
            if (!strncmp(segment, "include", 7)) {
                segment += 8;
                segment[strlen(segment) - 1] = '\0';

                Template *inc = templateNew(segment);
                inc->context = template->context;
                char *incBs = templateRender(inc);

                bsLCat(&res, incBs);
                bsDel(incBs);
                free(inc);
            } else if (!strncmp(segment, "when", 4)) {
                segment += 5;
                char *spc = strchr(segment, ' ');
                *spc = '\0';
                char *incBs = kvFindList(template->context, segment);

                if (incBs) {  // Found
                    segment = spc + 1;
                    spc = strchr(segment, ' ');
                    *spc = '\0';

                    if (!strcmp(incBs, segment)) {
                        segment = spc + 1;
                        segment[strlen(segment) - 1] = '\0';

                        bsLCat(&res, segment);
                    }
                }
            } else {
                fprintf(stderr, "error: unknown exp {%%%s} in '%s'\n", segment,
                        template->filename);
                exit(1);
            }
        } else {
            rep = false;

            bsLCat(&res, "{");
        }

        segment = strtok_r(NULL, "{\0", &pos);

        if (!segment)
            break;

        if (rep) {
            rep = false;
            segment += 1;
        } else {
            bsLCat(&res, "}");
        }

        bsLCat(&res, segment);
    }

    free(buff);
    return res;
}

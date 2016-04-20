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
    if (template->context) kvDelList(template->context);

    free(template);
}

void templateSet(Template *template, char *key, char *value)
{
    template->context = listCons(kvNew(key, value), sizeof(KV),
                                 template->context);
}

char *templateRender(Template *template)
{
    Template *inc;
    FILE *file = fopen(template->filename, "r");
    char *res  = bsNew("");
    char *pos, *buff, *incBs, *val;
    char *segment;
    bool  rep = false;
    size_t len;

    if (!file) {
        fprintf(stderr,
                "error: template '%s' not found\n", template->filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    rewind(file);

    buff = malloc(sizeof(char) * (len + 2));
    fread(buff + 1, sizeof(char), len, file);
    fclose(file);

    buff[0]       =  ' ';
    buff[len + 1] = '\0';

    // VARIABLES
    segment = strtok_r(buff, "{\0", &pos);
    assert(segment);
    bsLCat(&res, segment + 1);

    for (;;) {
        segment = strtok_r(NULL, "}\0", &pos);

        if (!segment) break;

        if (*segment == '{') {
            rep = true;
            segment += 1;
            val = kvFindList(template->context, segment);
            if (val)
                bsLCat(&res, val);
        } else if (*segment == '%') {
            rep = true;
            segment += 1;
            if (!strncmp(segment, "include", 7)) {
                segment += 8;
                segment[strlen(segment) - 1] = '\0';

                inc          = templateNew(segment);
                inc->context = template->context;
                incBs        = templateRender(inc);

                bsLCat(&res, incBs);
                bsDel(incBs);
                free(inc);
            } else if (!strncmp(segment, "when", 4)) {
                char *spc;

                segment += 5;
                spc      = strchr(segment, ' ');
                *spc     = '\0';
                incBs    = kvFindList(template->context, segment);

                if (incBs) {
                    segment = spc + 1;
                    spc     = strchr(segment, ' ');
                    *spc    = '\0';

                    if (!strcmp(incBs, segment)) {
                        segment = spc + 1;
                        segment[strlen(segment) - 1] = '\0';

                        bsLCat(&res, segment);
                    }
                }
            } else {
                fprintf(stderr,
                        "error: unknown exp {%%%s} in '%s'\n",
                        segment, template->filename);
                exit(1);
            }
        } else {
            rep = false;

            bsLCat(&res, "{");
        }

        segment = strtok_r(NULL, "{\0", &pos);

        if (!segment) break;

        if (rep) {
            rep      = false;
            segment += 1;
        } else {
            bsLCat(&res, "}");
        }

        bsLCat(&res, segment);
    }

    free(buff);

    return res;
}

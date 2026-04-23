#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "file.h"

char *slurp_file_into_malloced_cstr(const char *file_path)
{
    FILE *f = NULL;
    char *buffer = NULL;
    long size;

    f = fopen(file_path, "r");
    if (f == NULL)
        goto fail;
    if (fseek(f, 0, SEEK_END) < 0)
        goto fail;

    size = ftell(f);
    if (size < 0)
        goto fail;

    buffer = (char *)malloc(size + 1);
    if (buffer == NULL)
        goto fail;

    if (fseek(f, 0, SEEK_SET) < 0)
        goto fail;

    fread(buffer, 1, size, f);
    if (ferror(f))
        goto fail;

    buffer[size] = '\0';

    if (f)
    {
        fclose(f);
        errno = 0;
    }
    return buffer;
fail:
    if (f)
    {
        int saved_errno = errno;
        fclose(f);
        errno = saved_errno;
    }
    if (buffer)
    {
        free(buffer);
    }
    return NULL;
}
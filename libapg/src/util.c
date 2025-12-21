// NurOS Ruzen42 2025 apg/util.c
// Last change: Dec 21

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

char*
concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


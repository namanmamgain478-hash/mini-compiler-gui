#include <stdio.h>
#include "errors.h"

void print_errors(const ErrorList *list)
{
    if (!list || list->count == 0)
    {
        printf("No errors.\n");
        return;
    }

    printf("\n=== Errors ===\n");

    for (size_t i = 0; i < list->count; i++)
    {
        printf("Line %d, Col %d: %s\n",
               list->items[i].line,
               list->items[i].col,
               list->items[i].message);
    }
}
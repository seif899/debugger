#ifndef BREAKPOINT_LIST_H
#define BREAKPOINT_LIST_H

#include "breakpoint.h"
#include <stddef.h>
#include <sys/types.h>

typedef struct {
    Breakpoint *items;
    size_t count;
    size_t capacity;
} BreakpointList;

int breakpoint_list_init(BreakpointList *list);
void breakpoint_list_destroy(BreakpointList *list);

Breakpoint *breakpoint_list_add(
    BreakpointList *list,
    pid_t pid,
    uintptr_t address
);

Breakpoint *breakpoint_list_find(
    BreakpointList *list,
    uintptr_t address
);

int breakpoint_list_remove(
    BreakpointList *list,
    pid_t pid,
    uintptr_t address
);

#endif
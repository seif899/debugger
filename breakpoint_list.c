#include "breakpoint_list.h"

Breakpoint *breakpoint_list_find(BreakpointList *list, uintptr_t address){
    for (size_t i = 0; i < list->count; i++)
    {
        if (list->items[i].address == address){
            return &list->items[i];
        }
    }
    return NULL;
    
}
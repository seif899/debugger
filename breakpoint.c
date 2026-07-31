#include "breakpoint.h"

int breakpoint_enable(pid_t pid, Breakpoint *breakpoint){
    long word = ptrace(PTRACE_PEEKTEXT, pid, breakpoint->address, 0);
    breakpoint->saved_byte = word & 0xff;

    long patched = (word & ~0xff) | 0xcc;
    ptrace(PTRACE_POKETEXT, pid, breakpoint->address, patched);
}

int breakpoint_disable(pid_t pid, Breakpoint *breakpoint){

}

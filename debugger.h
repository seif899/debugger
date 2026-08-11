#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <sys/user.h>
#include <string.h>
#include "breakpoint_list.h"
#include <inttypes.h>

#define BREAKPOINTS_SIZE 100

typedef enum {
    DBG_STOPPED,
    DBG_RUNNING,
    DBG_STEPPING,
    DBG_RESUMING_BREAKPOINT,
    DBG_EXITED
} DebuggerState;

typedef struct {
    pid_t pid;
    struct user_regs_struct regs;
    BreakpointList *breakpoints;
    DebuggerState state;
    Breakpoint *current_breakpoint;
} Debugger;




void debugger_init(Debugger *, char *program);
void debugger_wait(Debugger *);
void debugger_handle_signal(Debugger *dbg, int status);
void debugger_handle_sigtrap(Debugger *);
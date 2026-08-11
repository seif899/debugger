#include "debugger.h"

void debugger_init(Debugger *dbg, char *program){
    dbg->state = DBG_STOPPED;
    dbg->pid = fork(); 
    if ( dbg->pid==0 ){
        ptrace(PT_TRACE_ME);
        printf("%s\n",program);
        char *env[] = { NULL }; 
        char *args[] = {program, NULL};
        execve(program, args, env);

        perror("error");
        exit(EXIT_FAILURE);
    } else {
        debugger_wait(dbg);
    }

}

void debugger_wait(Debugger *dbg){
    int status;
    waitpid(dbg->pid, &status, WUNTRACED);
    debugger_handle_signal(dbg, status);
}

void debugger_handle_signal(Debugger *dbg, int status){
    if (WIFEXITED(status)) {
        dbg->state = DBG_EXITED;
        printf("[Debugger] Program exited.\n");
        return;
    }

    if (WIFSTOPPED(status)){
        int sig = WSTOPSIG(status);

        switch(sig){
        case SIGTRAP:
            debugger_handle_sigtrap(dbg);
            break;

        case SIGSEGV:
            dbg->state = DBG_STOPPED;
            printf("[Debugger] Segmentation fault\n");
            break;

        default:
            printf("[Debugger] Stopped by signal %d\n", sig);
            break;
        }
    }
}

void debugger_continue(Debugger *dbg){
    if (dbg->current_breakpoint != NULL) {
        dbg->state == DBG_RESUMING_BREAKPOINT;
        ptrace(PTRACE_SINGLESTEP, dbg->pid, NULL, NULL);
        debugger_wait(dbg);
        dbg->current_breakpoint = NULL;
    }
    

    dbg->state = DBG_RUNNING;
    ptrace(PTRACE_CONT, dbg->pid, NULL, NULL);
    debugger_wait(dbg);
}

void debugger_step(Debugger *dbg){
    dbg->state = DBG_STEPPING;
    ptrace(PTRACE_SINGLESTEP, dbg->pid, NULL, NULL);
    debugger_wait(dbg);
}

void debugger_set_breakpoint(Debugger *dbg, uintptr_t address){
    Breakpoint *bp;
    bp->address = address;
    breakpoint_enable(dbg->pid, bp);
}

void debugger_delete_breakpoint(Debugger *dbg, Breakpoint *bp){
    breakpoint_disable(dbg->pid, bp);
    dbg->regs.rip--;
    ptrace(PTRACE_SETREGS, dbg->pid, 0, &dbg->regs);
    
}


void debugger_handle_sigtrap(Debugger *dbg){
    if (dbg->state == DBG_STEPPING){
        dbg->state = DBG_STOPPED;
        return;
    }

    if (dbg->state == DBG_RESUMING_BREAKPOINT){
        breakpoint_enable(dbg->pid,dbg->current_breakpoint);
        return;
    }

    if (dbg->state == DBG_RUNNING){
        Breakpoint *bp = breakpoint_list_find(dbg->breakpoints, dbg->regs.rip -1);
        if (bp){
            dbg->current_breakpoint = bp;
            printf("[Debugger] reached breakpoint at: 0x%llx\n", dbg->regs.rip);
            debugger_delete_breakpoint(dbg,bp->address);
            dbg->state = DBG_STOPPED;
            return;
        }
        
    }
    
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>

int main(int argc, char *argv[]){
    pid_t pid;
    char *program = argv[0];

    pid = fork(); 
    if ( pid==0 ){
        ptrace(PT_TRACE_ME);
        execve(program, "", "");
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP){
            printf("program is stopped");
        }
    }
    return 0;
}
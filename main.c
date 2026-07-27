#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <sys/user.h>
#include <string.h>

pid_t pid;
struct user_regs_struct regs;


void program_flow(int status){
    char input_buffer[256];
    while (1){
        printf("[Debugger] enter command: ");
        scanf("%255s", input_buffer);

        if (strcmp(input_buffer,"nexti") == 0){ 
            ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
            waitpid(pid, &status, WUNTRACED);
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            printf("[Debugger] rip:0x%llx\n",regs.rip);
        } else if (strcmp(input_buffer,"continue") == 0){
            ptrace(PTRACE_CONT, pid, NULL, NULL);
            break;
        }
    }
}

int main(int argc, char *argv[]){
    
    char input_buffer[256]; 
    char *program = NULL;
    if ( argc > 1 ){
        program = argv[1];
    } else {
        printf("[Debugger] Enter program name: ");
        if ( scanf("%255s", input_buffer) == 1 ){
            program = input_buffer;
        } 
    }
    pid = fork(); 
    if ( pid==0 ){
        ptrace(PT_TRACE_ME);
        printf("%s\n",program);
        char *env[] = { NULL }; 
        char *args[] = {program, NULL};
        execve(program, args, env);

        perror("error");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP){
            printf("[Debugger] process is stopped\n");
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            printf("[Debugger] rip:0x%llx\n",regs.rip);  
            program_flow(status);
            printf("[Debugger] resuming execution...\n");

        }


        waitpid(pid, &status, WUNTRACED);
        printf("[Debugger] Target process completed exit.\n");
    }
    return 0;
}
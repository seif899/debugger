#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <sys/user.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {

    uintptr_t address;

    uint8_t saved_byte;

    bool enabled;

} Breakpoint;

int breakpoint_enable(pid_t pid, Breakpoint *);
int breakpoint_disable(pid_t pid, Breakpoint *);


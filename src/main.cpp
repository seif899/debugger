#include <iostream>
#include <string>
#include "command.hpp"
#include "debugger.hpp"

int main(int argc, char *argv[]) {
    std::string program;
    if (argc > 1) {
        program = argv[1];
    } else {
        std::cout << "[Debugger] Enter program name: " << std::flush;
        if (!std::getline(std::cin, program) || program.empty()) return 1;
    }

    Debugger dbg(program);
    if (!dbg.start()) return 1;

    while (!dbg.finished()) {
        Command cmd = command_read();
        dbg.execute(cmd);
    }
    return 0;
}
#pragma once
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/user.h>
#include "breakpoint.hpp"
#include "command.hpp"

enum class DebuggerState { NotStarted, Stopped, Exited };

class Debugger {
public:
    explicit Debugger(std::string program) : program_(std::move(program)) {}
    ~Debugger();

    bool start();                       
    void execute(const Command &cmd);   
    bool finished() const { return quit_; }

private:
    // execution control
    void cont();
    void step();
    void step_over_breakpoint();
    void wait_for_stop();
    void handle_status(int status);
    void on_sigtrap();

    // breakpoints
    void set_breakpoint(uint64_t addr);
    void delete_breakpoint(uint64_t addr);
    void list_breakpoints() const;

    // inspection
    bool refresh_regs();
    void print_regs();
    void set_reg(const std::string &name, uint64_t value);
    void examine(uint64_t addr, size_t words);
    void load_base();

    bool require_stopped() const;
    bool resolve(const std::string &arg, uint64_t &addr) const;

    std::string program_;
    pid_t pid_ = -1;
    DebuggerState state_ = DebuggerState::NotStarted;
    user_regs_struct regs_{};
    uint64_t base_ = 0;                 
    std::map<uint64_t, Breakpoint> breakpoints_;
    bool quit_ = false;
};
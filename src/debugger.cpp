#include "debugger.hpp"
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

Debugger::~Debugger() {
    if (pid_ > 0 && state_ != DebuggerState::Exited) {
        kill(pid_, SIGKILL);
        waitpid(pid_, nullptr, 0);
    }
}

bool Debugger::start() {
    pid_ = fork();
    if (pid_ < 0) { perror("fork"); return false; }

    if (pid_ == 0) {
        personality(ADDR_NO_RANDOMIZE);          // stable addresses between runs
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            perror("ptrace(TRACEME)");
            _exit(127);
        }
        execl(program_.c_str(), program_.c_str(), (char *)nullptr);
        perror("execl");
        _exit(127);
    }

    int status;
    if (waitpid(pid_, &status, 0) < 0) { perror("waitpid"); return false; }
    if (!WIFSTOPPED(status)) {
        std::cerr << "[Debugger] Could not start '" << program_ << "'\n";
        state_ = DebuggerState::Exited;
        return false;
    }
    ptrace(PTRACE_SETOPTIONS, pid_, nullptr, PTRACE_O_EXITKILL);
    state_ = DebuggerState::Stopped;
    load_base();
    refresh_regs();
    std::printf("[Debugger] Started '%s' (pid %d), load base 0x%lx, stopped at 0x%llx\n",
                program_.c_str(), pid_, base_, regs_.rip);
    return true;
}

// The first mapping in /proc/<pid>/maps is the executable's load address.
void Debugger::load_base() {
    std::ifstream maps("/proc/" + std::to_string(pid_) + "/maps");
    std::string line;
    if (std::getline(maps, line)) base_ = std::stoull(line, nullptr, 16);
}

bool Debugger::refresh_regs() {
    return ptrace(PTRACE_GETREGS, pid_, nullptr, &regs_) == 0;
}

bool Debugger::require_stopped() const {
    if (state_ == DebuggerState::Stopped) return true;
    std::cout << "[Debugger] Program is not running.\n";
    return false;
}

// "0x401136" is absolute, "+0x1139" is relative to the load base.
bool Debugger::resolve(const std::string &arg, uint64_t &addr) const {
    bool rel;
    if (!parse_address(arg, addr, rel)) {
        std::cout << "[Debugger] Bad address: " << arg << "\n";
        return false;
    }
    if (rel) addr += base_;
    return true;
}

// ---------------------------------------------------------------- execution

void Debugger::wait_for_stop() {
    int status;
    if (waitpid(pid_, &status, 0) < 0) { perror("waitpid"); return; }
    handle_status(status);
}

void Debugger::handle_status(int status) {
    if (WIFEXITED(status)) {
        state_ = DebuggerState::Exited;
        std::printf("[Debugger] Program exited with code %d.\n", WEXITSTATUS(status));
        return;
    }
    if (WIFSIGNALED(status)) {
        state_ = DebuggerState::Exited;
        std::printf("[Debugger] Program killed by signal %d (%s).\n",
                    WTERMSIG(status), strsignal(WTERMSIG(status)));
        return;
    }
    if (!WIFSTOPPED(status)) return;

    int sig = WSTOPSIG(status);
    state_ = DebuggerState::Stopped;
    refresh_regs();
    if (sig == SIGTRAP) {
        on_sigtrap();
    } else {
        std::printf("[Debugger] Stopped by signal %d (%s) at 0x%llx\n",
                    sig, strsignal(sig), regs_.rip);
    }
}

void Debugger::on_sigtrap() {
    // After an INT3, rip points one byte past the breakpoint address.
    auto it = breakpoints_.find(regs_.rip - 1);
    if (it != breakpoints_.end() && it->second.enabled()) {
        regs_.rip -= 1;
        ptrace(PTRACE_SETREGS, pid_, nullptr, &regs_);
        std::printf("[Debugger] Hit breakpoint at 0x%llx\n", regs_.rip);
    } else {
        std::printf("[Debugger] Stopped at 0x%llx\n", regs_.rip);
    }
}

// If we are sitting on an enabled breakpoint, execute the original instruction
// with the breakpoint temporarily removed, then put it back.
void Debugger::step_over_breakpoint() {
    auto it = breakpoints_.find(regs_.rip);
    if (it == breakpoints_.end() || !it->second.enabled()) return;

    it->second.disable();
    ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr);
    int status;
    waitpid(pid_, &status, 0);
    if (WIFEXITED(status) || WIFSIGNALED(status)) { handle_status(status); return; }
    it->second.enable();
    refresh_regs();
}

void Debugger::cont() {
    if (!require_stopped()) return;
    step_over_breakpoint();
    if (state_ != DebuggerState::Stopped) return;
    ptrace(PTRACE_CONT, pid_, nullptr, nullptr);
    wait_for_stop();
}

void Debugger::step() {
    if (!require_stopped()) return;
    auto it = breakpoints_.find(regs_.rip);
    bool on_bp = it != breakpoints_.end() && it->second.enabled();
    if (on_bp) it->second.disable();
    ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr);
    wait_for_stop();
    if (on_bp && state_ == DebuggerState::Stopped) it->second.enable();
}

// -------------------------------------------------------------- breakpoints

void Debugger::set_breakpoint(uint64_t addr) {
    if (!require_stopped()) return;
    if (breakpoints_.count(addr)) {
        std::printf("[Debugger] Breakpoint already set at 0x%lx\n", addr);
        return;
    }
    auto [it, _] = breakpoints_.emplace(addr, Breakpoint(pid_, addr));
    if (!it->second.enable()) {
        std::printf("[Debugger] Cannot write breakpoint at 0x%lx: %s\n", addr, strerror(errno));
        breakpoints_.erase(it);
        return;
    }
    std::printf("[Debugger] Breakpoint set at 0x%lx\n", addr);
}

void Debugger::delete_breakpoint(uint64_t addr) {
    auto it = breakpoints_.find(addr);
    if (it == breakpoints_.end()) {
        std::printf("[Debugger] No breakpoint at 0x%lx\n", addr);
        return;
    }
    if (state_ == DebuggerState::Stopped) it->second.disable();
    breakpoints_.erase(it);
    std::printf("[Debugger] Breakpoint at 0x%lx deleted\n", addr);
}

void Debugger::list_breakpoints() const {
    if (breakpoints_.empty()) { std::cout << "No breakpoints.\n"; return; }
    int n = 1;
    for (auto &[addr, bp] : breakpoints_)
        std::printf("%d  0x%lx (+0x%lx)  %s\n", n++, addr, addr - base_,
                    bp.enabled() ? "enabled" : "disabled");
}

// --------------------------------------------------------------- inspection

#define REG_LIST(X) \
    X(rip) X(rsp) X(rbp) X(rax) X(rbx) X(rcx) X(rdx) X(rsi) X(rdi) \
    X(r8) X(r9) X(r10) X(r11) X(r12) X(r13) X(r14) X(r15) X(eflags)

void Debugger::print_regs() {
    if (!require_stopped()) return;
    refresh_regs();
#define X(r) std::printf("%-7s 0x%016llx  %llu\n", #r, regs_.r, regs_.r);
    REG_LIST(X)
#undef X
}

void Debugger::set_reg(const std::string &name, uint64_t value) {
    if (!require_stopped()) return;
    refresh_regs();
    bool found = false;
#define X(r) if (name == #r) { regs_.r = value; found = true; }
    REG_LIST(X)
#undef X
    if (!found) { std::cout << "[Debugger] Unknown register: " << name << "\n"; return; }
    ptrace(PTRACE_SETREGS, pid_, nullptr, &regs_);
    std::printf("[Debugger] %s = 0x%lx\n", name.c_str(), value);
}

void Debugger::examine(uint64_t addr, size_t words) {
    if (!require_stopped()) return;
    for (size_t i = 0; i < words; ++i) {
        uint64_t a = addr + i * 8;
        errno = 0;
        long w = ptrace(PTRACE_PEEKDATA, pid_, a, nullptr);
        if (w == -1 && errno) {
            std::printf("0x%016lx: <cannot read: %s>\n", a, strerror(errno));
            return;
        }
        std::printf("0x%016lx: 0x%016lx\n", a, (unsigned long)w);
    }
}

// ----------------------------------------------------------------- dispatch

void Debugger::execute(const Command &cmd) {
    uint64_t addr = 0;
    switch (cmd.type) {
    case CmdType::Empty: break;
    case CmdType::Help:
        std::cout <<
            "  break|b <addr>      set breakpoint (hex; '+off' is relative to load base)\n"
            "  delete|d <addr>     remove breakpoint\n"
            "  info|i              list breakpoints\n"
            "  continue|c          resume execution\n"
            "  step|s              execute one instruction\n"
            "  regs                show registers\n"
            "  set <reg> <hex>     write a register\n"
            "  x <addr> [n]        examine n 8-byte words (default 1)\n"
            "  base                show load base\n"
            "  quit|q              exit\n";
        break;
    case CmdType::Run:
        std::cout << "[Debugger] Program is already started; use 'continue'.\n"; break;
    case CmdType::Continue: cont(); break;
    case CmdType::Step:     step(); break;
    case CmdType::Break:
        if (cmd.args.empty()) std::cout << "usage: break <addr>\n";
        else if (resolve(cmd.args[0], addr)) set_breakpoint(addr);
        break;
    case CmdType::Delete:
        if (cmd.args.empty()) std::cout << "usage: delete <addr>\n";
        else if (resolve(cmd.args[0], addr)) delete_breakpoint(addr);
        break;
    case CmdType::InfoBreak: list_breakpoints(); break;
    case CmdType::Regs:      print_regs(); break;
    case CmdType::SetReg: {
        uint64_t v; bool rel;
        if (cmd.args.size() < 2 || !parse_address(cmd.args[1], v, rel))
            std::cout << "usage: set <reg> <hex value>\n";
        else set_reg(cmd.args[0], v);
        break;
    }
    case CmdType::Examine: {
        if (cmd.args.empty()) { std::cout << "usage: x <addr> [n]\n"; break; }
        size_t n = cmd.args.size() > 1 ? std::strtoul(cmd.args[1].c_str(), nullptr, 0) : 1;
        if (resolve(cmd.args[0], addr)) examine(addr, n ? n : 1);
        break;
    }
    case CmdType::Base:
        std::printf("load base: 0x%lx\n", base_); break;
    case CmdType::Quit: quit_ = true; break;
    case CmdType::Unknown:
        std::cout << "Unknown command. Type 'help'.\n"; break;
    }
    if (state_ == DebuggerState::Exited && !quit_) {
        std::cout << "[Debugger] Session finished.\n";
        quit_ = true;
    }
}
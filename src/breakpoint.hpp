#pragma once
#include <cstdint>
#include <sys/types.h>

// A software breakpoint: replaces the first byte at `addr` with INT3 (0xCC).
class Breakpoint {
public:
    Breakpoint(pid_t pid, uintptr_t addr) : pid_(pid), addr_(addr) {}

    bool enable();
    bool disable();

    bool enabled() const { return enabled_; }
    uintptr_t address() const { return addr_; }

private:
    pid_t pid_;
    uintptr_t addr_;
    uint8_t saved_ = 0;
    bool enabled_ = false;
};
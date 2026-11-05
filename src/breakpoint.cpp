#include "breakpoint.hpp"
#include <cerrno>
#include <sys/ptrace.h>

bool Breakpoint::enable() {
    if (enabled_) return true;
    errno = 0;
    long word = ptrace(PTRACE_PEEKTEXT, pid_, addr_, nullptr);
    if (word == -1 && errno) return false;
    saved_ = static_cast<uint8_t>(word & 0xff);
    long patched = (word & ~0xffL) | 0xcc;
    if (ptrace(PTRACE_POKETEXT, pid_, addr_, patched) == -1) return false;
    enabled_ = true;
    return true;
}

bool Breakpoint::disable() {
    if (!enabled_) return true;
    errno = 0;
    long word = ptrace(PTRACE_PEEKTEXT, pid_, addr_, nullptr);
    if (word == -1 && errno) return false;
    // Restore only the original byte; keep the other 7 bytes of the word intact.
    long restored = (word & ~0xffL) | saved_;
    if (ptrace(PTRACE_POKETEXT, pid_, addr_, restored) == -1) return false;
    enabled_ = false;
    return true;
}
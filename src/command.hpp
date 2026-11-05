#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class CmdType {
    Empty, Help, Run, Continue, Step, Break, Delete, InfoBreak,
    Regs, SetReg, Examine, Base, Quit, Unknown
};

struct Command {
    CmdType type = CmdType::Empty;
    std::vector<std::string> args;
    std::string raw;
};

// Reads one line from stdin and parses it. Returns Quit on EOF.
Command command_read(const char *prompt = "(mydbg) ");
Command command_parse(const std::string &line);

// Parses "0x401136", "401136" or "+0x1139" (base-relative). Returns false on error.
bool parse_address(const std::string &s, uint64_t &out, bool &relative);
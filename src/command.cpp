#include "command.hpp"
#include <iostream>
#include <sstream>
#include <unordered_map>

Command command_parse(const std::string &line) {
    Command c;
    c.raw = line;
    std::istringstream iss(line);
    std::string word;
    if (!(iss >> word)) return c;
    while (iss >> c.args.emplace_back()) {}
    c.args.pop_back();

    static const std::unordered_map<std::string, CmdType> table = {
        {"help", CmdType::Help},      {"h", CmdType::Help},
        {"run", CmdType::Run},        {"r", CmdType::Run},
        {"continue", CmdType::Continue}, {"c", CmdType::Continue},
        {"step", CmdType::Step},      {"s", CmdType::Step},
        {"break", CmdType::Break},    {"b", CmdType::Break},
        {"delete", CmdType::Delete},  {"d", CmdType::Delete},
        {"info", CmdType::InfoBreak}, {"i", CmdType::InfoBreak},
        {"regs", CmdType::Regs},
        {"set", CmdType::SetReg},
        {"x", CmdType::Examine},
        {"base", CmdType::Base},
        {"quit", CmdType::Quit},      {"q", CmdType::Quit},
    };
    auto it = table.find(word);
    c.type = it == table.end() ? CmdType::Unknown : it->second;
    return c;
}

Command command_read(const char *prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        Command c;
        c.type = CmdType::Quit;
        return c;
    }
    return command_parse(line);
}

bool parse_address(const std::string &s, uint64_t &out, bool &relative) {
    relative = false;
    std::string t = s;
    if (!t.empty() && t[0] == '*') t.erase(0, 1);
    if (!t.empty() && t[0] == '+') { relative = true; t.erase(0, 1); }
    if (t.empty()) return false;
    try {
        size_t pos = 0;
        out = std::stoull(t, &pos, 16);  // addresses are always hex
        return pos == t.size();
    } catch (...) {
        return false;
    }
}
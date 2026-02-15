#pragma once

struct ParsedArgs {

    using Value = std::variant<int, bool, std::string, std::filesystem::path>;

    std::unordered_map<std::string, Value> path_flags;
    std::unordered_set<std::string> boolean_flags;

};

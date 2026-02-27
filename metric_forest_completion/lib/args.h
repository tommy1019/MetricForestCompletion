#pragma once

#include <algorithm>
#include <optional>
#include <print>
#include <string>

template <typename T>
bool parse_arg(int argc, char** argv, std::string name, T& t, std::optional<char> short_name = {}, bool required = true) {

    std::string arg_name = "--" + name;
    std::optional<std::string> arg_short_name = {};
    if (short_name.has_value()) {
        arg_short_name = "-";
        arg_short_name.value().push_back(short_name.value());
    }

    bool found_arg = false;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], arg_name.c_str()) == 0) || (arg_short_name.has_value() && strcmp(argv[i], arg_short_name->c_str()) == 0)) {
            found_arg = true;

            static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, bool> || std::is_same_v<T, int>);

            if constexpr (std::is_same_v<T, std::string>) {
                t = argv[i + 1];
            }

            if constexpr (std::is_same_v<T, bool>) {
                std::string str = argv[i + 1];
                std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
                if (str == "true" || str == "yes")
                    t = true;
                else if (str == "false" || str == "no")
                    t = false;
                else
                    return false;
            }

            if constexpr (std::is_same_v<T, int>) {
                t = std::stod(argv[i + 1]);
            }
        }

        if (argv[i][0] != '-') {
            std::print("Error: unused argument: {}\n", argv[i]);
            return false;
        }

        i++;
    }

    if (!required)
        return true;

    if (!found_arg) {
        std::print("Error: missing required argument: {}\n", name);
    }

    return found_arg;
}

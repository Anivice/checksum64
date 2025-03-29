#include "argument_parser.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <type_traits>

void replace_all(std::string& original, const std::string& target, const std::string& replacement)
{
    if (target.empty()) return; // Avoid infinite loop if target is empty

    if (target.size() == 1 && replacement.empty()) {
        // Special case: single character target, empty replacement
        std::erase_if(original, [&](const char c) { return c == target[0]; });
    } else {
        // General case: replace all occurrences of target with replacement
        size_t pos = 0;
        while ((pos = original.find(target, pos)) != std::string::npos) {
            original.replace(pos, target.length(), replacement);
            pos += replacement.length(); // Move past the replacement
        }
    }
}

std::vector<std::string> splitString(const std::string& input, const char delimiter = ',')
{
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }

    return result;
}

bool Arguments::if_predefined_arg_contains(const std::string& name)
{
    return std::ranges::any_of(predefined_args, [&name](const auto& arg) {
        return std::to_string(arg.short_name) == name || arg.name == name;
    });
}

std::string Arguments::get_full_name(const char ch)
{
    const auto it = std::ranges::find_if(predefined_args, [ch](const auto& arg) {
        return arg.short_name == ch;
    });

    if (it != predefined_args.end()) {
        return it->name;
    }

    char buff[2]{};
    buff[0] = ch;
    throw std::invalid_argument("Argument for `" + std::string(buff) + "` does not exist");
}

Arguments::single_arg_t Arguments::get_single_arg_by_fullname(const std::string & name)
{
    const auto it =
        std::ranges::find_if(predefined_args, [name](const auto& arg)
        { return arg.name == name; }
        );

    if (it != predefined_args.end()) {
        return *it;
    }

    throw std::invalid_argument("Argument for `" + name + "` does not exist");
}

Arguments::Arguments(const int argc, const char* argv[], predefined_args_t predefined_args_)
    : predefined_args(std::move(predefined_args_))
{
    bool last_is_an_option = false;
    std::string arg_key_str;

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == nullptr) {
            throw std::invalid_argument("NULL terminated before processing all arguments");
        }

        std::string this_arg = argv[i];
        if (last_is_an_option) {
            arguments[arg_key_str].push_back(this_arg);
            last_is_an_option = false;
        }
        else if (this_arg[0] == '-' && this_arg.size() != 1)
        {
            // Handle argument with key
            replace_all(this_arg, "-", ""); // Remove leading '-'

            if (this_arg.find('=') != std::string::npos)
            {
                const auto list = splitString(this_arg, '=');
                if (list.size() != 2) {
                    throw std::invalid_argument("In `" + this_arg + "`, a key is provided but its value cannot be parsed");
                }
                arguments[list[0]].push_back(list[1]);
            }
            else
            {
                single_arg_t pre_def_arg;
                std::string full_name = this_arg;
                if (this_arg.size() == 1) {
                    full_name = get_full_name(this_arg[0]);
                    pre_def_arg = get_single_arg_by_fullname(full_name);
                } else {
                    pre_def_arg = get_single_arg_by_fullname(this_arg);
                }

                last_is_an_option = pre_def_arg.value_required;
                arg_key_str = full_name;
                arguments[full_name]; // create an entry
            }
        }
        else {
            // Handle bare argument
            arguments["BARE"].push_back(this_arg);
        }
    }

    for (const auto&[key, val] : arguments) {
        if (key != "BARE" && get_single_arg_by_fullname(key).value_required && val.empty()) {
            throw std::invalid_argument("Key `" + key + "` expects a value");
        }
    }
}

template<typename T>
concept Numeric = std::is_arithmetic_v<T>;  // This will allow both integral and floating-point types

template < Numeric Type >
Type max_of(const typename std::vector<Type>::iterator& begin, const typename std::vector<Type>::iterator& end)
{
    Type result = *begin;
    for (auto it = begin; it != end; ++it)
    {
        if (*it > result) {
            result = *it;
        }
    }

    return result;
}

void Arguments::print_help() const
{
    std::vector<std::pair < std::string, std::string > > arg_name_list;
    std::vector<uint64_t> arg_name_len_list;
    for (const auto& arg : predefined_args)
    {
        std::stringstream ss;
        ss << "-" << arg.short_name << ",--" << arg.name;
        arg_name_list.emplace_back(ss.str(), arg.explanation);
        arg_name_len_list.emplace_back(arg_name_list.back().first.size());
    }

    const auto max_len = max_of<uint64_t>(arg_name_len_list.begin(), arg_name_len_list.end());
    for (const auto& [arg_name, explanation] : arg_name_list)
    {
        constexpr uint64_t padding_size = 4;
        std::cout << "    " << arg_name
                  << std::string(std::max(
                      static_cast<signed long long>(padding_size)
                      + (static_cast<signed long long>(max_len)
                          - static_cast<signed long long>(arg_name.size())),
                      1ll),
                      ' ')
                  << explanation << std::endl;
    }
}

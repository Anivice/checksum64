#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <map>
#include <string>
#include <vector>

class Arguments {
public:
    using args_t = std::map < std::string, std::vector < std::string > >;

    struct single_arg_t {
        std::string name;
        char short_name = 0;
        bool value_required;
        std::string explanation;
    };

    using predefined_args_t = std::vector < single_arg_t >;

private:
    args_t arguments;
    predefined_args_t predefined_args;
    bool if_predefined_arg_contains(const std::string &);
    std::string get_full_name(char);
    single_arg_t get_single_arg_by_fullname(const std::string &);

public:
    explicit Arguments(int argc, const char *argv[], predefined_args_t);
    [[nodiscard]] explicit operator args_t() const {
        return this->arguments;
    }

    void print_help() const;
};

#endif //ARGUMENT_PARSER_H

#include "log.hpp"
#include "argument_parser.h"
#include "bin2hex.h"
#include <algorithm>
#include <cctype>
#include <vector>
#include <stdexcept>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <cstring>

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
# include <windows.h>
# ifdef __DEBUG__
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
# endif // __DEBUG__
#endif // WIN32

class CRC64 {
public:
    CRC64() {
        init_crc64();
    }

    void update(const uint8_t* data, const size_t length) {
        for (size_t i = 0; i < length; ++i) {
            crc64_value = table[(crc64_value ^ data[i]) & 0xFF] ^ (crc64_value >> 8);
        }
    }

    [[nodiscard]] uint64_t get_checksum() const {
        return crc64_value;
    }

private:
    uint64_t crc64_value{};

    static uint64_t table[256];

    void init_crc64()
    {
        crc64_value = 0xFFFFFFFFFFFFFFFF;
        for (uint64_t i = 0; i < 256; ++i) {
            uint64_t crc = i;
            for (uint64_t j = 8; j--; ) {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xC96C5795D7870F42;  // Standard CRC-64 polynomial
                else
                    crc >>= 1;
            }
            table[i] = crc;
        }
    }
};

uint64_t CRC64::table[256] = {};

Arguments::predefined_args_t arguments = {
    Arguments::single_arg_t {
        .name = "checksum",
        .short_name = 'c',
        .value_required = true,
        .explanation = "Checksum file in the format <FILENAME>: <CHECKSUM>"
    },
    Arguments::single_arg_t {
        .name = "uppercase",
        .short_name = 'U',
        .value_required = false,
        .explanation = "Use uppercase hex value"
    },
    Arguments::single_arg_t {
        .name = "help",
        .short_name = 'h',
        .value_required = false,
        .explanation = "Show this help message"
    },
};

void replace_all(std::string&, const std::string&, const std::string&);
std::string getEnvVar(const std::string &);

bool uppercase = false;

#ifndef WIN32
uint64_t hash_a_file(std::string filename)
{
    replace_all(filename, "\\", "/");
#else
uint64_t hash_a_file(const std::string& filename)
{
#endif
    std::unique_ptr<std::istream> file_stream;
    if (filename != "STDIN") {
        file_stream = std::make_unique<std::ifstream>(filename, std::ios::in | std::ios::binary);
    } else {
        file_stream = std::make_unique<std::istream>(std::cin.rdbuf());
    }

    if (!file_stream || !*file_stream) {
        throw std::runtime_error("Could not open file " + filename);
    }

    std::array <uint8_t, 4 * 1024> buffer{};
    CRC64 crc64;
    do
    {
        file_stream->read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        const auto size = file_stream->gcount();
        if (size < 0)
        {
            throw std::runtime_error("Cannot read file " + filename);
        }

        if (size == 0) {
            debug::log(debug::to_stderr, debug::warning_log, filename + " is an empty file.\n");
        }

        crc64.update(buffer.data(), size);
    } while (*file_stream);

    return crc64.get_checksum();
}

bool is_utf8()
{
    const auto lang = getEnvVar("LANG");
    const auto lc_ctype = getEnvVar("LC_CTYPE");

    if (lang.find("UTF-8") != std::string::npos) {
        return true;
    }

    if (lc_ctype.find("UTF-8") != std::string::npos) {
        return true;
    }

#ifdef WIN32
    if (GetConsoleOutputCP() == 65001) {
        return true;
    }
#endif

    return false;
}

std::string remove_non_printable(const std::string& input)
{
    std::string output;
    for (const char ch : input) {
        if (std::isprint(static_cast<unsigned char>(ch))) {
            output += ch;  // Keep only printable characters
        }
    }
    return output;
}

int main(int argc, const char **argv)
{
#if defined(WIN32) && defined(__DEBUG__)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        const Arguments args(argc, argv, arguments);
        uppercase = static_cast<Arguments::args_t>(args).contains("uppercase");

        auto single_file_hash = [](const std::string & filename)->void
        {
            const uint64_t checksum = hash_a_file(filename);
            std::vector < char > data;
            data.resize(sizeof(checksum));
            (*reinterpret_cast<uint64_t *>(data.data())) = checksum;
            std::cout << filename << ": ";
            const auto hex = bin2hex::bin2hex(data);
            if (uppercase)
            {
                for (const char& ch : hex) {
                    std::cout.put(static_cast<char>(std::toupper(ch)));
                }
            } else {
                std::cout << hex;
            }
            std::cout << std::endl;
        };

        auto print_help = [&]()->void
        {
            std::cout << *argv << " [OPTIONS] FILE1 [[FILE2],...]" << std::endl;
            std::cout << "OPTIONS: " << std::endl;
            args.print_help();
        };

#ifdef WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif

        if (static_cast<Arguments::args_t>(args).contains("BARE"))
        {
            for (const auto flist = static_cast<Arguments::args_t>(args).at("BARE");
                const auto & filename : flist)
            {
                if (filename == "-") {
                    single_file_hash("STDIN");
                } else {
                    single_file_hash(filename);
                }
            }
        }
        else if (static_cast<Arguments::args_t>(args).contains("checksum"))
        {
            for (const auto flist = static_cast<Arguments::args_t>(args).at("checksum");
                const auto & filename : flist)
            {
                std::ifstream file_stream(filename);
                std::string line;
                while (std::getline(file_stream, line))
                {
                    const auto pos = line.find_last_of(':');
                    if (pos == std::string::npos || pos == 0) {
                        throw std::runtime_error("Invalid checksum file format");
                    }

                    const std::string fname = line.substr(0, pos);
                    std::string checksum = line.substr(pos + 1);
                    replace_all(checksum, " ", "");
                    const uint64_t real_checksum = hash_a_file(fname);
                    std::vector < char > data;
                    data.resize(sizeof(real_checksum));
                    (*reinterpret_cast<uint64_t *>(data.data())) = real_checksum;
                    const auto real_hex = bin2hex::bin2hex(data);
                    std::ranges::transform(checksum, checksum.begin(),
                                           [](const unsigned char c) {
                                               return std::tolower(c);
                                           });
                    checksum = remove_non_printable(checksum);
                    if (real_hex == checksum)
                    {
                        if (is_utf8()) {
                        #ifdef WIN32
                            SetConsoleOutputCP(CP_UTF8);
                        #endif // WIN32
                            std::vector<unsigned char> CheckMark = {0xE2, 0x9C, 0x94, 0xEf, 0xB8, 0x8F}; /* ✔️ */
                            std::cout.write(reinterpret_cast<const char*>(CheckMark.data()),
                                static_cast<signed long long>(CheckMark.size()));
                            std::cout  << "    " << fname << std::endl;
                        } else {
                            std::cout << "OK  " << fname << std::endl;
                        }
                    }
                    else
                    {
                        if (is_utf8()) {
                        #ifdef WIN32
                            SetConsoleOutputCP(CP_UTF8);
                        #endif // WIN32
                            std::vector<unsigned char> CrossMark = {0xE2, 0x9D, 0x8C}; /* ❌ */
                            std::cout.write(reinterpret_cast<const char*>(CrossMark.data()),
                                static_cast<signed long long>(CrossMark.size()));
                            std::cout  << "    " << fname << std::endl;
                        } else {
                            std::cout << "BAD  " << fname << std::endl;
                        }
                    }
                }
            }
        } else if (static_cast<Arguments::args_t>(args).contains("help")) {
            print_help();
            return EXIT_SUCCESS;
        } else {
            single_file_hash("STDIN");
            return EXIT_SUCCESS;
        }
    } catch (std::exception & e) {
        std::cerr << "ERROR: " << e.what();
        char buff[128] {};
#ifdef WIN32
        if (strerror_s(buff, sizeof(buff), errno) != 0)
#else
        if (strerror_r(errno, buff, sizeof(buff)) != 0)
#endif
        {
            std::cerr << "Unknown error" << std::endl;
            return EXIT_FAILURE;
        }
        std::cerr << ": " << buff << std::endl;
        return EXIT_FAILURE;
    }
}

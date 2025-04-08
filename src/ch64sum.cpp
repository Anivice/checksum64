/* ch64sum.cpp
 *
 * Copyright 2025 Anivice Ives
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#ifndef WIN32
#define _POSIX_C_SOURCE 200809L
#endif

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
#include <locale>

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
    Arguments::single_arg_t {
        .name = "version",
        .short_name = 'v',
        .value_required = false,
        .explanation = "Show version"
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
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::array <uint8_t, 4 * 1024> buffer{};
    CRC64 crc64;
    do
    {
        file_stream->read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        const auto size = file_stream->gcount();
        if (size < 0)
        {
            throw std::runtime_error("Cannot read file: " + filename);
        }

        if (size == 0 && crc64.get_checksum() == 0xFFFFFFFFFFFFFFFF) {
            debug::log(debug::to_stderr, debug::warning_log, filename + " is an empty file.\n");
        }

        crc64.update(buffer.data(), size);
    } while (*file_stream);

    return crc64.get_checksum();
}

bool is_utf8()
{
    static bool I_have_checked_and_is_true = false;

    if (I_have_checked_and_is_true) {
        return true;
    }

#ifdef WIN32
    // Enable if possible
    if (!SetConsoleOutputCP(CP_UTF8)) {
        return false;
    }
#endif // WIN32

    const auto lang = getEnvVar("LANG");
    const auto lc_ctype = getEnvVar("LC_CTYPE");

    if (lang.find("UTF-8") != std::string::npos) {
        I_have_checked_and_is_true = true;
        return true;
    }

    if (lc_ctype.find("UTF-8") != std::string::npos) {
        I_have_checked_and_is_true = true;
        return true;
    }

#ifdef WIN32
    if (GetConsoleOutputCP() == 65001) {
        I_have_checked_and_is_true = true;
        return true;
    }
#endif

    return false;
}

bool is_colorful()
{
    static bool I_have_checked_and_is_true = false;

    if (I_have_checked_and_is_true) {
        return true;
    }

#ifdef WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (!GetConsoleMode(h, &mode)) {
        return false;
    }

    // Enable if possible
    if (!SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return false;
    }

    // query again
    if (!GetConsoleMode(h, &mode)) {
        return false;
    }

    if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
        I_have_checked_and_is_true = true;
        return true;
    }
#endif

    const auto lang = getEnvVar("TERM");

    if (lang.find("xterm-256color") != std::string::npos) {
        I_have_checked_and_is_true = true;
        return true;
    }

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
            std::setlocale(LC_ALL, "C");
#if defined(WIN32)
            SetConsoleOutputCP(437);
#endif
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
            std::vector < std::string > good_files;
            std::vector < std::string > bad_files;
            uint64_t file_count = 0;

            for (const auto flist = static_cast<Arguments::args_t>(args).at("checksum");
                const auto & filename : flist)
            {
                std::ifstream file_stream(filename, std::ios::in);
                std::string line;

                if (!file_stream) {
                    throw std::runtime_error("Could not open file: " + filename);
                }

                while (std::getline(file_stream, line))
                {
                    const auto pos = line.find_last_of(':');
                    if (pos == std::string::npos || pos == 0) {
                        debug::log(debug::to_stderr, debug::error_log, "Invalid checksum file format\n");
                        continue; // skip ill-formatted parts
                    }

                    const std::string fname = line.substr(0, pos);
                    std::string checksum = line.substr(pos + 1);
                    replace_all(checksum, " ", "");
                    uint64_t real_checksum = 0;
                    try {
                        file_count++; // before checksum, increase file_count
                        real_checksum = hash_a_file(fname);
                    } catch (const std::exception & e) {
                        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n");
                    }
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
                        good_files.emplace_back(fname);
                        if (is_utf8()) {
                            std::vector<unsigned char> CheckMark = {0xE2, 0x9C, 0x94, 0xEf, 0xB8, 0x8F}; /* ✔️ */
                            std::cout.write(reinterpret_cast<const char*>(CheckMark.data()),
                                static_cast<signed long long>(CheckMark.size()));
                            std::cout  << "    "
                                       << (is_colorful() ? "\033[32;1m" + fname + "\033[0m" : fname)
                                       << std::endl;
                        } else {
                            if (is_colorful()) {
                                std::cout << "\033[32;1m" "OK  " + fname + "\033[0m" << std::endl;
                            } else {
                                std::cout << "OK  " << fname << std::endl;
                            }
                        }
                    }
                    else
                    {
                        bad_files.emplace_back(fname);
                        if (is_utf8()) {
                            std::vector<unsigned char> CrossMark = {0xE2, 0x9D, 0x8C}; /* ❌ */
                            std::cout.write(reinterpret_cast<const char*>(CrossMark.data()),
                                static_cast<signed long long>(CrossMark.size()));
                            std::cout  << "    "
                                       << (is_colorful() ? "\033[31;1m" + fname + "\033[0m" : fname)
                                       << std::endl;
                        } else {
                            if (is_colorful()) {
                                std::cout << "\033[31;1m" "BAD " + fname + "\033[0m" << std::endl;
                            } else {
                                std::cout << "BAD " << fname << std::endl;
                            }
                        }
                    }
                }
            }

            if (!bad_files.empty())
            {
                std::cout << "Checksum summary:" << std::endl;
                if (bad_files.size() > 1) {
                    std::cout << "These files failed the test: " << std::endl;
                } else {
                    std::cout << "This file failed the test: " << std::endl;
                }
                for (const auto & f : bad_files) {
                    std::cout << "    " << (is_colorful() ?  "\033[31;1m" : "")
                              << f << (is_colorful() ?  "\033[0m" : "") << std::endl;
                }
            }

            std::cout << "Checksum completed (Good/Bad/All) ("
                      << (is_colorful() ? "\033[32;1m" : "")
                      << good_files.size() << (is_colorful() ? "\033[0m" : "") << "/"
                      << (is_colorful() ? (bad_files.empty() ? "\033[32;1m" : "\033[31;1m") : "")
                      << bad_files.size() << (is_colorful() ? "\033[0m" : "") << "/"
                      << (is_colorful() ? "\033[34;1m" : "") << file_count << (is_colorful() ? "\033[0m" : "")
                      << ")" << std::endl;
            if (good_files.size() == file_count && bad_files.empty())
            {
                std::cout << (is_colorful() ? "\033[32;1m" : "")
                          << "File integrity ensured"
                          << (is_colorful() ? "\033[0m" : "")
                          << std::endl;
                return EXIT_SUCCESS;
            } else {
                std::cout << (is_colorful() ? "\033[31;1m" : "")
                          << "File integrity violated"
                          << (is_colorful() ? "\033[0m" : "")
                          << std::endl;
                return EXIT_FAILURE;
            }
        } else if (static_cast<Arguments::args_t>(args).contains("help")) {
            print_help();
            return EXIT_SUCCESS;
        } else if (static_cast<Arguments::args_t>(args).contains("version")) {
#if defined(WIN32)
            auto basename = [](const std::string & path)->std::string {
                return path.substr(path.find_last_of('\\') + 1);
            };
            std::cout << basename(*argv);
#else  // defined(WIN32)
            std::cout << *argv;
#endif // defined(WIN32)
            std::cout << " [CRC64 ";
            if (is_colorful()) {
                std::cout << "\033[0;1;4;7m" "NON-STANDARD" "\033[0m";
            } else {
                std::cout <<  "**NON-STANDARD**";
            }
            std::cout << " CHECKSUM] Version " CRC64_VERSION << std::endl;
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
        {
            std::cerr << "Unknown error" << std::endl;
            return EXIT_FAILURE;
        }
        std::cerr << ": " << buff << std::endl;
        return EXIT_FAILURE;
#else
        auto ret = strerror_r(errno, buff, sizeof(buff));
        std::cerr << ": " << ret << std::endl;
        return EXIT_FAILURE;
#endif
    }
}

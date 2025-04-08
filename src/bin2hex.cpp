/* bin2hex.cpp
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

#include "bin2hex.h"
#include "log.hpp"

#include <stdexcept>

constexpr int max_line_char_num = 64;

namespace bin2hex {
    char hex_table [] = {
        '0', 0x00,
        '1', 0x01,
        '2', 0x02,
        '3', 0x03,
        '4', 0x04,
        '5', 0x05,
        '6', 0x06,
        '7', 0x07,
        '8', 0x08,
        '9', 0x09,
        'a', 0x0A,
        'b', 0x0B,
        'c', 0x0C,
        'd', 0x0D,
        'e', 0x0E,
        'f', 0x0F,
    };

    void c_bin2hex(const char bin, char hex[2])
    {
        auto find_in_table = [](const char p_hex) -> char {
            for (size_t i = 0; i < sizeof(hex_table); i += 2) {
                if (hex_table[i + 1] == p_hex) {
                    return hex_table[i];
                }
            }

            throw std::invalid_argument("Invalid binary code");
        };

        const char bin_a = static_cast<char>(bin >> 4) & 0x0F;
        const char bin_b = bin & 0x0F;

        hex[0] = find_in_table(bin_a);
        hex[1] = find_in_table(bin_b);
    }

    std::string bin2hex(const std::vector < char > & vec)
    {
        int cur_line_char_num = 0;

        std::string result;
        char buffer [3] { };
        for (const auto & bin : vec) {
            c_bin2hex(bin, buffer);
            result += buffer;
            cur_line_char_num += 2;

            if (cur_line_char_num == max_line_char_num) {
                result += "\n";
                cur_line_char_num = 0;
            }
        }

        return result;
    }
}

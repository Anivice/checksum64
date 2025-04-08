/* bin2hex.h
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

#ifndef BIN2HEX_H
#define BIN2HEX_H

#include <vector>
#include <string>

namespace bin2hex {
    void c_bin2hex(char bin, char hex[2]);

    std::string bin2hex(const std::vector < char > &);
    inline std::string bin2hex(const std::string & str) {
        const std::vector < char > vec(str.begin(), str.end());
        return bin2hex::bin2hex(vec);
    }
} // bin2hex

#endif //BIN2HEX_H

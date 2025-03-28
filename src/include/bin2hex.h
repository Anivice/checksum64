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

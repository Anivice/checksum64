#include "log.hpp"
#include "argument_parser.h"
#include "bin2hex.h"
#include <cstring>
#include <vector>
#include <stdexcept>
#include <array>
#include <fstream>

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
};

bool uppercase = false;

uint64_t hash_a_file(const std::string & filename)
{
    std::unique_ptr < std::istream > file = nullptr;
    if (filename != "STDIN") {
        file = std::make_unique<std::ifstream>(filename, std::ifstream::binary);
    } else {
        file.reset(&std::cin);
    }

    std::array <uint8_t, 4 * 1024> buffer{};
    CRC64 crc64;
    while (!file->eof())
    {
        file->read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        const auto size = file->gcount();
        if (size < 0)
        {
            if (filename == "STDIN") {
                auto ptr = file.release();
            }
            throw std::runtime_error("Cannot read file");
        }

        crc64.update(buffer.data(), size);
    }

    if (filename == "STDIN") {
        auto ptr = file.release();
    }

    return crc64.get_checksum();
}

int main(int argc, const char **argv)
{
    try
    {
        const Arguments args(argc, argv, arguments);
        uppercase = static_cast<Arguments::args_t>(args).contains("uppercase");
        if (static_cast<Arguments::args_t>(args).contains("BARE"))
        {
            for (const auto flist = static_cast<Arguments::args_t>(args).at("BARE");
                const auto & filename : flist)
            {
                const uint64_t checksum = hash_a_file(filename);
                std::vector < char > data;
                data.resize(sizeof(checksum));
                (*(uint64_t*)data.data()) = checksum;
                std::cout << filename << ": " << bin2hex::bin2hex(data) << std::endl;
            }
        }
        else if (static_cast<Arguments::args_t>(args).contains("checksum")) {
            // ...
        } else if (static_cast<Arguments::args_t>(args).empty()) {
            args.print_help();
        }
    } catch (std::exception & e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

# CRC64 Checksum Utility

A simple CRC64 checksum tool

```bash
    crc64sum [OPTIONS] FILE1 [[FILE2],...]
    OPTIONS:
        -c,--checksum     Checksum file in the format <FILENAME>: <CHECKSUM>
        -U,--uppercase    Use uppercase hex value
        -e,--endian       Endianness, acceptable options are little or big (default)
        -h,--help         Show this help message
        -v,--version      Show version
        -a,--clear        Disable color codes and UTF-8 codes
```

> Note: --checksum accepts a checksum summary from the output of `crc64sum FILE1 [[FILE2],...]`.

> Note2: When no file provided or file name is STDIN or `-`, `crc64sum` reads from standard input.

> Note3:
> Endianness can be different for different checksum programs.
> For example, 7-Zip produces checksum with big endianness in its utility set,
> hence the reason why this utility uses big endianness by default.
> However, little endianness is more often than not preferred by many other utilities.
> `crc64sum` accepts two arguments for option `--endian`: `little` and `big`.

# How to build

This tool supports both Linux and Windows and uses CMake to bridge different building systems.

To build this tool, use the following command:

```bash
git clone https://github.com/Anivice/checksum64 --depth=1 && mkdir checksum64/build && cd checksum64/build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release
```

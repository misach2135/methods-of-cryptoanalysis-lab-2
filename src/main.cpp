#include <iostream>
#include <fstream>
#include <sstream>
#include <clocale>

#include "text_processor.hpp"
#include "encoding.hpp"

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

int main(int argc, char *argv[])
{
    // if (argc < 2)
    // {
    //     std::cerr << "Missing filename in args" << std::endl;
    //     return -1;
    // }

    // const std::string_view filename = std::string_view(argv[1]);

    // std::ifstream f(filename.data());

    // if (f.fail())
    // {
    //     std::cerr << "Failed to open the file: " << filename << std::endl;
    //     return -1;
    // }

    // std::stringstream buff;

    // buff << f.rdbuf();

    // std::string origin_text = buff.str();

    // std::cout << origin_text << std::endl;

    // text_processor::preprocessText(origin_text);

    // std::cout << "Origin test: " << origin_text << std::endl;

    const char codepoint[2] = {char(0xd0), char(0x91)};
    // const char codepoint[2] = {char(0x1), char(0x0)};

    std::cout << convert_cp1251_to_utf8(convert_utf8_to_cp1251(0xd091)) << std::endl;

    return 0;
}
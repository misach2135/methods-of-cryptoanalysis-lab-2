#include <iostream>
#include <fstream>
#include <sstream>
#include <clocale>

#include "text_processor.hpp"

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

int main(int argc, char *argv[])
{
    std::setlocale(LC_ALL, "cp-1251");

    if (argc < 2)
    {
        std::cerr << "Missing filename in args" << std::endl;
        return -1;
    }

    std::ifstream f(argv[1]);
    std::stringstream buff;

    std::cout << "Hello world" << std::endl;
    std::cout << argv[1] << std::endl;

    buff << f.rdbuf();

    std::string origin_text = buff.str();

    std::cout << origin_text << std::endl;

    text_processor::preprocessText(origin_text);

    std::cout << "Origin test: " << origin_text << std::endl;

    return 0;
}
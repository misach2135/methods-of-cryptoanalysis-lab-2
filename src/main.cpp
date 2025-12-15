#include <iostream>
#include <fstream>
#include <sstream>

#include "text_processor.hpp"

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

int main(int argc, char *argv[])
{
    std::ifstream f(argv[1]);
    std::stringstream buff;

    std::cout << argv[1] << std::endl;

    buff << f.rdbuf();

    std::string origin_text = buff.str();

    text_processor::preprocessText(origin_text);

    std::cout << origin_text << std::endl;

    return 0;
}
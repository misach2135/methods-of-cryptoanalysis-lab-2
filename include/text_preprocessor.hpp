#ifndef TEXT_PREPROCESSOR_HPP
#define TEXT_PREPROCESSOR_HPP

#include <string>

namespace text_preprocessor
{
    void replaceSymbols(std::string &text, char from, char to);
    void toLowercase(std::string &text);

    void preprocessText(std::string &text);
}

#endif
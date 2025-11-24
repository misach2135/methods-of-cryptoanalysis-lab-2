#ifndef TEXT_PREPROCESSOR_HPP
#define TEXT_PREPROCESSOR_HPP

#include <string>
#include <algorithm>

namespace text_preprocessor
{
    void removeSymbols(std::string &text, const std::string &symbols);
    void preprocessText(std::string &text);
}

#endif
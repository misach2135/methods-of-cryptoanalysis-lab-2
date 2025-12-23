#ifndef TEXT_PROCESSOR_HPP
#define TEXT_PROCESSOR_HPP

#include <string>
#include <algorithm>

#include "encoding.hpp"

namespace text_processor
{
    void removeSymbols(std::string &text, const std::string &symbols);
    void preprocessText(std::string &text);
}

#endif
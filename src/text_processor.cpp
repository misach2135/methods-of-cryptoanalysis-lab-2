#include "text_processor.hpp"

void text_preprocessor::removeSymbols(std::string &text, const std::string &symbols)
{
    for (auto c : symbols)
    {
        std::remove(text.begin(), text.end(), c);
    }
}

void text_preprocessor::preprocessText(std::string &text)
{
    // Make text lowercase
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    // Replace orphan symbols
    std::replace(text.begin(), text.end(), 'ґ', 'г');

    // Remove redundant symbols
    text_preprocessor::removeSymbols(text, "', .:;-");
}

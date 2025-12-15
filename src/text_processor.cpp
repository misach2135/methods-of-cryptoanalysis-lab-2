#include "text_processor.hpp"

void text_processor::removeSymbols(std::string &text, const std::string &symbols)
{
}

void text_processor::preprocessText(std::string &text)
{
    // Make text lowercase
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    // Replace orphan symbols
    std::replace(text.begin(), text.end(), char(180), char(227));

    // Remove redundant symbols
    const std::string symbols = "\', .:;-\"";

    for (auto c : symbols)
    {
        std::remove(text.begin(), text.end(), c);
    }
}

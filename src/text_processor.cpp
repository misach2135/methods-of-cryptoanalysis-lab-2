#include "text_processor.hpp"

void text_processor::preprocessText(std::string &text)
{
    // Make text lowercase
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    // Replace orphan symbols
    std::replace(text.begin(), text.end(), 'ґ', 'г');

    // Remove redundant symbols
    const std::string symbols = "\', .:;-\"";

    for (auto c : symbols)
    {
        std::remove(text.begin(), text.end(), c);
    }
}

#include "text_processor.hpp"
void text_processor::preprocessText(std::string &text)
{
    for (auto it = text.begin(); it != text.end(); it++)
    {
        next_utf8_char()
    }
}

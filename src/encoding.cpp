#include "encoding.hpp"

#include <iostream>
#include <bitset>

// C - char in little-endian byte order
uint8_t convert_utf8_to_cp1251(uint16_t c)
{
    const uint16_t c0 = ((c & 0x1f00) >> 2);
    const uint16_t c1 = c & 0x003f;

    const uint16_t utf16_symbol = c0 | c1;

    if (utf16_symbol < 128)
    {
        return utf16_symbol;
    }

    return convert_letter_utf16_to_cp1251(utf16_symbol);
}

uint16_t convert_cp1251_to_utf8(uint8_t c)
{
    if (!((c >= 0 && c < 128) || (c >= CP1251_CYRILLIC_CAPITAL_A && c <= CP1251_CYRILLIC_SMALL_YA)))
    {
        return 0;
    }
    const uint8_t offset_cp1251 = c - CP1251_CYRILLIC_CAPITAL_A;
    const uint16_t utf16_symbol = UTF16BE_CYRILLIC_CAPITAL_A + uint16_t(offset_cp1251);

    const uint16_t c0 = 0x07c0 & utf16_symbol;
    const uint16_t c1 = 0x003f & utf16_symbol;

    return 0xC000 | (c0 << 2) | 0x80 | c1;
}

uint8_t convert_letter_utf16_to_cp1251(uint16_t c)
{
    return uint8_t((c - UTF16BE_CYRILLIC_CAPITAL_A) + CP1251_CYRILLIC_CAPITAL_A);
}

std::string_view::iterator next_utf8_char(std::string_view sv, std::string_view::iterator it)
{
    return std::string_view::iterator();
}

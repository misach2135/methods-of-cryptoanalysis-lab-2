#include "encoding.hpp"

#include <iostream>
#include <bitset>
#include <optional>

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

std::optional<std::tuple<std::string_view::iterator, uint16_t>> next_utf8_char(std::string_view::const_iterator it, std::string_view::const_iterator end)
{
    if (it == end)
        return std::nullopt;

    if (*it <= 0x7f)
    {
        return std::make_tuple(it + 1, *it);
    }
    if (*it & 0b11000000)
    {
        if (end - it < 2)
            return std::nullopt;
        uint16_t c0 = *it & 0b00011111;
        uint16_t c1 = *(it + 1) & 0b00111111;

        uint16_t utf16_symbol = (c0 << 5) | c1;

        return std::make_tuple(it + 2, utf16_symbol);
    }
    if (*it & 0b11100000)
    {
        if (end - it < 3)
            return std::nullopt;
        // Currently, not supported
        return std::make_tuple(it + 3, 0);
    }
    if (*it & 0b11110000)
    {
        if (end - it < 4)
            return std::nullopt;
        // Currently, not supported
        return std::make_tuple(it + 4, 0);
    }
    return std::nullopt;
}

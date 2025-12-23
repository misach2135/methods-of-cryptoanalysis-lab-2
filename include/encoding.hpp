#ifndef ENCODING_HPP
#define ENCODING_HPP

#include <cstdint>
#include <functional>

const uint16_t UTF16BE_CYRILLIC_CAPITAL_A = 0x0410;
const uint16_t UTF16BE_CYRILLIC_SMALL_YA = 0x044f;

const uint8_t CP1251_CYRILLIC_CAPITAL_A = 0xc0;
const uint8_t CP1251_CYRILLIC_SMALL_YA = 0xff;

uint8_t convert_utf8_to_cp1251(uint16_t);
uint16_t convert_cp1251_to_utf8(uint8_t);
uint8_t convert_letter_utf16_to_cp1251(uint16_t);

std::optional<std::tuple<std::string_view::iterator, uint16_t>> next_utf8_char(std::string_view::const_iterator it, std::string_view::const_iterator end);

#endif
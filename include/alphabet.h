#ifndef LAB2_ALPHABET
#define LAB2_ALPHABET

#include <cstdint>

const uint8_t CYRILLIC_CODEPAGE_PREFIX = 0x04;
const uint16_t UKR_G_LETTER = 0x0491;
const uint16_t UKR_H_LETTER = 0x0433;
const uint32_t ALPHABET_SIZE = 32;
const char32_t* UKRAINIAN_ALPHABET = U"абвгдеєжзиіїйклмнопрстуфхцчшщьюя";
const char32_t* UKRAINIAN_ALPHABET_END = UKRAINIAN_ALPHABET + 33;

#endif
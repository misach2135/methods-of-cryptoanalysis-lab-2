#ifndef LAB2_ALPHABET
#define LAB2_ALPHABET

#include <cstdint>

inline constexpr uint8_t CYRILLIC_CODEPAGE_PREFIX = 0x04;
inline constexpr uint16_t UKR_G_LETTER = 0x0491;
inline constexpr uint16_t UKR_H_LETTER = 0x0433;
inline constexpr uint32_t ALPHABET_SIZE = 32;
inline constexpr char32_t* UKRAINIAN_ALPHABET =
    U"абвгдеєжзиіїйклмнопрстуфхцчшщьюя";
inline constexpr char32_t* UKRAINIAN_ALPHABET_END = UKRAINIAN_ALPHABET + 33;

#endif
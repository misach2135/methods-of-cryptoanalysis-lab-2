#ifndef LAB_1_TEXT_PROCESSOR
#define LAB_1_TEXT_PROCESSOR

#include <unicode/uchar.h>

#include <cstdint>
#include <string>

namespace lab2 {

extern const uint8_t CYRILLIC_CODEPAGE_PREFIX;
extern const uint16_t UKR_G_LETTER;
extern const uint16_t UKR_H_LETTER;
extern const char32_t* UKRAINIAN_ALPHABET;

std::string prepareText(std::string text);

}  // namespace lab2

#endif
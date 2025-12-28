#ifndef LAB_1_TEXT_PROCESSOR
#define LAB_1_TEXT_PROCESSOR

#include <unicode/uchar.h>

#include <cstdint>
#include <string>
#include <vector>

namespace lab2 {

extern const uint8_t CYRILLIC_CODEPAGE_PREFIX;
extern const uint16_t UKR_G_LETTER;
extern const uint16_t UKR_H_LETTER;
extern const char32_t* UKRAINIAN_ALPHABET;

uint8_t cyrillicUnicodeToByte(uint32_t codepoint);
uint32_t byteToCyrillicUnicode(uint8_t byte);

std::string prepareText(std::string text);
std::string bytesToCyrillicText(std::vector<uint8_t> bytes);
std::vector<uint8_t> cyrillicTextToBytes(std::string text);

}  // namespace lab2

#endif
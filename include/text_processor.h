#ifndef LAB_1_TEXT_PROCESSOR
#define LAB_1_TEXT_PROCESSOR

#include <unicode/uchar.h>

#include <cstdint>
#include <string>
#include <vector>

namespace lab2 {

uint8_t cyrillicUnicodeToByte(uint32_t codepoint);
uint32_t byteToCyrillicUnicode(uint8_t byte);

std::string prepareText(const std::string& text);
std::string bytesToCyrillicText(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> cyrillicTextToBytes(const std::string& text);

std::vector<uint8_t> generateRandomText(size_t len);

}  // namespace lab2

#endif
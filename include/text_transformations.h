#ifndef LAB2_TEXT_TRANSFORMATIONS
#define LAB2_TEXT_TRANSFORMATIONS

#include <cstdint>
#include <vector>

namespace lab2 {

std::vector<uint8_t> applyVigenereCipher(const std::vector<uint8_t>& bytes,
                                         const std::vector<uint8_t>& key);

std::vector<uint8_t> applyAphineLetterSubstitution(
    const std::vector<uint8_t>& bytes, uint16_t a, uint16_t b);

std::vector<uint8_t> applyAphineBigramSubstitution(
    const std::vector<uint8_t>& bytes, uint8_t a, uint8_t b);

}  // namespace lab2

#endif
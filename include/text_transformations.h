#ifndef LAB2_TEXT_TRANSFORMATIONS
#define LAB2_TEXT_TRANSFORMATIONS

#include <array>
#include <cstdint>
#include <vector>

#include "alphabet.h"

namespace lab2 {

template <const size_t KeySize>
std::vector<uint8_t> applyVigenereCipher(
    const std::vector<uint8_t>& bytes,
    const std::array<uint8_t, KeySize>& key) {
  if (key.empty()) {
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
  }

  std::vector<uint8_t> result;
  result.reserve(bytes.size());

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    auto value =
        static_cast<uint8_t>((bytes[i] + key[i % key.size()]) % ALPHABET_SIZE);
    result.push_back(value);
  }

  return result;
}

std::vector<uint8_t> applyAphineLetterSubstitution(
    const std::vector<uint8_t>& bytes, uint8_t a, uint8_t b);

std::vector<uint8_t> applyAphineBigramSubstitution(
    const std::vector<uint8_t>& bytes, uint16_t a, uint16_t b);

}  // namespace lab2

#endif

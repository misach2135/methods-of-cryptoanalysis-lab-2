#include "text_transformations.h"

#include "alphabet.h"

std::vector<uint8_t> lab2::applyAphineLetterSubstitution(
    const std::vector<uint8_t>& bytes, uint8_t a, uint8_t b) {
  constexpr uint16_t M = ALPHABET_SIZE;
  std::vector<uint8_t> result;
  result.reserve(bytes.size());

  for (auto byte : bytes) {
    auto value =
        static_cast<uint8_t>((static_cast<uint32_t>(a) * byte + b) % M);
    result.push_back(value);
  }

  return result;
}

std::vector<uint8_t> lab2::applyAphineBigramSubstitution(
    const std::vector<uint8_t>& bytes, uint16_t a, uint16_t b) {
  constexpr uint16_t M = ALPHABET_SIZE;
  constexpr uint16_t M2 = M * M;

  std::vector<uint8_t> result(bytes.begin(), bytes.end());
  for (std::size_t i = 0; i + 1 < bytes.size(); i += 2) {
    auto bigram = static_cast<uint16_t>(bytes[i] * M + bytes[i + 1]);
    auto transformed =
        static_cast<uint16_t>((static_cast<uint32_t>(a) * bigram + b) % M2);

    result[i] = static_cast<uint8_t>(transformed / M);
    result[i + 1] = static_cast<uint8_t>(transformed % M);
  }

  return result;
}

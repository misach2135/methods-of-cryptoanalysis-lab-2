#include "text_transformations.h"

std::vector<uint8_t> lab2::applyVigenereCipher(
    const std::vector<uint8_t>& bytes, const std::vector<uint8_t>& key) {
  if (key.empty()) {
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
  }

  constexpr uint8_t kAlphabetSize = 33;
  std::vector<uint8_t> result;
  result.reserve(bytes.size());

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    auto value =
        static_cast<uint8_t>((bytes[i] + key[i % key.size()]) % kAlphabetSize);
    result.push_back(value);
  }

  return result;
}

std::vector<uint8_t> lab2::applyAphineLetterSubstitution(
    const std::vector<uint8_t>& bytes, uint16_t a, uint16_t b) {
  constexpr uint16_t kAlphabetSize = 33;
  std::vector<uint8_t> result;
  result.reserve(bytes.size());

  for (auto byte : bytes) {
    auto value = static_cast<uint8_t>(
        (static_cast<uint32_t>(a) * byte + b) % kAlphabetSize);
    result.push_back(value);
  }

  return result;
}

std::vector<uint8_t> lab2::applyAphineBigramSubstitution(
    const std::vector<uint8_t>& bytes, uint8_t a, uint8_t b) {
  constexpr uint16_t kAlphabetSize = 33;
  constexpr uint16_t kBigramMod = kAlphabetSize * kAlphabetSize;

  std::vector<uint8_t> result(bytes.begin(), bytes.end());
  for (std::size_t i = 0; i + 1 < bytes.size(); i += 2) {
    auto bigram =
        static_cast<uint16_t>(bytes[i] * kAlphabetSize + bytes[i + 1]);
    auto transformed = static_cast<uint16_t>(
        (static_cast<uint32_t>(a) * bigram + b) % kBigramMod);

    result[i] = static_cast<uint8_t>(transformed / kAlphabetSize);
    result[i + 1] = static_cast<uint8_t>(transformed % kAlphabetSize);
  }

  return result;
}

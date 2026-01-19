#include "text_processor.h"

#include <alphabet.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <random>

#include "utf8.h"

inline uint8_t lab2::cyrillicUnicodeToByte(uint32_t codepoint) {
  auto it = std::find(UKRAINIAN_ALPHABET.begin(), UKRAINIAN_ALPHABET.end(),
                      static_cast<char32_t>(codepoint));

  if (it == UKRAINIAN_ALPHABET.end()) {
    return 0xFF;
  }

  return static_cast<uint8_t>(it - UKRAINIAN_ALPHABET.begin());
}

inline uint32_t lab2::byteToCyrillicUnicode(uint8_t byte) {
  if (byte >= 33) {
    return static_cast<uint32_t>('?');
  }
  return static_cast<uint32_t>(UKRAINIAN_ALPHABET[byte]);
}

// 1. Літера Г замінена на Г +
// 2. Видалені спецсимволи
// 3. Текст містить лише маленькі літери алфавіту
std::string lab2::prepareText(const std::string& text) {
  std::string filtered_text;

  for (auto it = text.begin(); it != text.end();) {
    uint32_t codepoint = utf8::next(it, text.end());

    if (!u_isalpha(static_cast<UChar32>(codepoint))) {
      continue;
    }

    auto lowercase_codepoint =
        static_cast<char32_t>(u_tolower(static_cast<UChar32>(codepoint)));

    if (lowercase_codepoint == UKR_G_LETTER) {
      lowercase_codepoint = UKR_H_LETTER;
    }

    if (std::find(UKRAINIAN_ALPHABET.begin(), UKRAINIAN_ALPHABET.end(),
                  lowercase_codepoint) == UKRAINIAN_ALPHABET.end()) {
      continue;
    }

    utf8::append(lowercase_codepoint, std::back_inserter(filtered_text));
  }

  return filtered_text;
}

std::string lab2::bytesToCyrillicText(const std::vector<uint8_t>& bytes) {
  std::string result;
  result.reserve(bytes.size());

  for (auto byte : bytes) {
    auto codepoint = byteToCyrillicUnicode(byte);
    utf8::append(codepoint, std::back_inserter(result));
  }

  return result;
}

std::vector<uint8_t> lab2::cyrillicTextToBytes(const std::string& text) {
  std::vector<uint8_t> v;
  v.reserve(text.length());
  auto text_end = text.end();

  for (auto it = text.begin(); it != text_end;) {
    auto codepoint = utf8::next(it, text_end);
    auto byte = cyrillicUnicodeToByte(codepoint);
    v.push_back(byte);
  }

  return v;
}

std::vector<uint8_t> lab2::generateRandomText(size_t len) {
  // TODO: Move to another place?
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, ALPHABET_SIZE);

  std::vector<uint8_t> v;

  while (len--) {
    uint8_t a = distrib(gen);
    uint8_t b = distrib(gen);
    v.push_back((a + b) % (ALPHABET_SIZE + 1));
  }

  return v;
}

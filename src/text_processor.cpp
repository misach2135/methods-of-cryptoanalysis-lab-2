#include "text_processor.h"

#include <alphabet.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>

#include "utf8.h"

inline uint8_t lab2::cyrillicUnicodeToByte(uint32_t codepoint) {
  auto alphabet_end = UKRAINIAN_ALPHABET + 33;
  auto it = std::find(UKRAINIAN_ALPHABET, alphabet_end,
                      static_cast<char32_t>(codepoint));
  return static_cast<uint8_t>(it - UKRAINIAN_ALPHABET);
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
    auto prev_it = it;
    uint32_t codepoint = utf8::next(it, text.end());

    if (!u_isalpha(static_cast<UChar32>(codepoint))) {
      continue;
    }

    auto lowercase_codepoint =
        static_cast<char32_t>(u_tolower(static_cast<UChar32>(codepoint)));

    if (lowercase_codepoint == UKR_G_LETTER) {
      lowercase_codepoint = UKR_H_LETTER;
    }

    if (std::find(UKRAINIAN_ALPHABET, UKRAINIAN_ALPHABET_END,
                  lowercase_codepoint) == UKRAINIAN_ALPHABET_END) {
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

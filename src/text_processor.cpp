#include "text_processor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>

#include "utf8.h"

const uint8_t lab2::CYRILLIC_CODEPAGE_PREFIX = 0x04;
const uint16_t lab2::UKR_G_LETTER = 0x0491;
const uint16_t lab2::UKR_H_LETTER = 0x0433;
const char32_t* lab2::UKRAINIAN_ALPHABET = U"абвгдеєжзиіїйклмнопрстуфхцчшщьюя";

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

    if (lowercase_codepoint == lab2::UKR_G_LETTER) {
      lowercase_codepoint = lab2::UKR_H_LETTER;
    }

    auto alphabet_end = UKRAINIAN_ALPHABET + 33;
    if (std::find(lab2::UKRAINIAN_ALPHABET, alphabet_end,
                  lowercase_codepoint) == alphabet_end) {
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

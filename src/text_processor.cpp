#include "text_processor.h"

#include <algorithm>
#include <iostream>

#include "utf8.h"

const uint8_t lab2::CYRILLIC_CODEPAGE_PREFIX = 0x04;
const uint16_t lab2::UKR_G_LETTER = 0x0491;
const uint16_t lab2::UKR_H_LETTER = 0x0433;
const char32_t* lab2::UKRAINIAN_ALPHABET = U"абвгдеєжзиіїйклмнопрстуфхцчшщьюя";

// 1. Літера Г замінена на Г +
// 2. Видалені спецсимволи
// 3. Текст містить лише маленькі літери алфавіту
std::string lab2::processText(std::string text) {
  for (auto it = text.begin(); it != text.end();) {
    auto prev_it = it;
    uint32_t codepoint = utf8::next(it, text.end());

    if (!u_isalpha(static_cast<UChar32>(codepoint))) {
      it = text.erase(prev_it, it);
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
      it = text.erase(prev_it, it);
      continue;
    }

    std::string repl;
    utf8::append(lowercase_codepoint, std::back_inserter(repl));

    text.replace(prev_it, it, repl);
  }
  return text;
}
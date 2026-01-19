#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

// Variant 4

#include "alphabet.h"
#include "statistics.h"
#include "text_processor.h"
#include "text_transformations.h"

constexpr uint32_t L_ARR[] = {10, 100, 1000, 10000};
constexpr uint32_t N_ARR[] = {10000, 10000, 10000, 1000};

struct Chunk {
  uint8_t a1;
  uint8_t b1;
  uint16_t a2;
  uint16_t b2;

  std::array<uint8_t, 1> vigenere_key_1;
  std::array<uint8_t, 5> vigenere_key_5;
  std::array<uint8_t, 10> vigenere_key_10;

  std::vector<uint8_t> plain_text;

  std::vector<uint8_t> vigenere_text_1;
  std::vector<uint8_t> vigenere_text_5;
  std::vector<uint8_t> vigenere_text_10;
  std::vector<uint8_t> affine_symbol_text;
  std::vector<uint8_t> affine_bigram_text;
};

int lab(const std::string& filepath);

int main(int argc, char* argv[]) {
  if (argc != 2) {
    spdlog::error("Error: Path to the text is required.");
    return -1;
  }

  return lab(argv[1]);
}

int lab(const std::string& filepath) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> uniform_distribution(0, ALPHABET_SIZE);

  std::ifstream file(filepath);

  if (!file.is_open()) {
    spdlog::error("Error: Can't open the file {}", filepath);
    return 1;
  }

  file.seekg(0, std::ios::end);
  std::size_t size = file.tellg();
  file.seekg(0);

  std::string text(size, '\0');
  file.read(text.data(), size);

  spdlog::info("Text is loaded. Text size in bytes: {}", text.size());

  spdlog::info("Start preprocessing.");
  std::string processedText = lab2::prepareText(text);
  spdlog::info("Preprocessing end. Text size after preprocessing: {}.",
               processedText.size());

  // Since every letter is on cyrrylic page, we can use only the last bytes of
  // letters. This may reduce memory usage.
  spdlog::info("Converting text to bytes.");
  auto bytes_vec = lab2::cyrillicTextToBytes(processedText);
  spdlog::info("Bytes count(in practice -- letters count): {}.",
               bytes_vec.size());

  spdlog::info("Calculating statistics.");
  auto statistics = lab2::calculateStatistics(bytes_vec);

  spdlog::info("Calculated statistics of preprocessed text:\n{}", statistics);

  std::vector<Chunk> chunks;
  auto cursor = bytes_vec.begin();

  spdlog::info("Start creating chunks");

  for (int i = 0; i < 4; i++) {
    uint32_t l = L_ARR[i];
    uint32_t n = N_ARR[i];

    spdlog::info("L = {}", l);
    for (uint32_t j = 0; j < n; j++) {
      std::vector<uint8_t> buff(l);

      std::copy(cursor, cursor + l, buff.data());
      cursor += l;

      std::array<uint8_t, 1> vigenere_key_1 = {uniform_distribution(gen)};

      std::array<uint8_t, 5> vigenere_key_5 = {
          uniform_distribution(gen), uniform_distribution(gen),
          uniform_distribution(gen), uniform_distribution(gen),
          uniform_distribution(gen),
      };
      std::array<uint8_t, 10> vigenere_key_10 = {
          uniform_distribution(gen), uniform_distribution(gen),
          uniform_distribution(gen), uniform_distribution(gen),
          uniform_distribution(gen), uniform_distribution(gen),
          uniform_distribution(gen), uniform_distribution(gen),
          uniform_distribution(gen), uniform_distribution(gen),
      };

      uint8_t a1 = uniform_distribution(gen);
      uint8_t b1 = uniform_distribution(gen);
      uint16_t a2 = static_cast<uint16_t>(uniform_distribution(gen)) *
                    static_cast<uint16_t>(uniform_distribution(gen));
      uint16_t b2 = static_cast<uint16_t>(uniform_distribution(gen)) *
                    static_cast<uint16_t>(uniform_distribution(gen));

      std::vector<uint8_t> vigenere_text_1;
      std::vector<uint8_t> vigenere_text_5;
      std::vector<uint8_t> vigenere_text_10;
      std::vector<uint8_t> affine_symbol_text;
      std::vector<uint8_t> affine_bigram_text;

      auto t1 = std::thread([&vigenere_text_1, &buff, &vigenere_key_1]() {
        vigenere_text_1 = lab2::applyVigenereCipher(buff, vigenere_key_1);
      });

      auto t2 = std::thread([&vigenere_text_5, &buff, &vigenere_key_5]() {
        vigenere_text_5 = lab2::applyVigenereCipher(buff, vigenere_key_5);
      });

      auto t3 = std::thread([&vigenere_text_10, &buff, &vigenere_key_10]() {
        vigenere_text_10 = lab2::applyVigenereCipher(buff, vigenere_key_10);
      });

      auto t4 = std::thread([&affine_symbol_text, &buff, a1, b1]() {
        affine_symbol_text = lab2::applyAphineLetterSubstitution(buff, a1, b1);
      });

      auto t5 = std::thread([&affine_bigram_text, &buff, a2, b2]() {
        affine_bigram_text = lab2::applyAphineBigramSubstitution(buff, a2, b2);
      });

      t1.join();
      t2.join();
      t3.join();
      t4.join();
      t5.join();

      chunks.push_back({
          a1,
          b1,
          a2,
          b2,
          std::move(vigenere_key_1),
          std::move(vigenere_key_5),
          std::move(vigenere_key_10),
          std::move(buff),
          std::move(vigenere_text_1),
          std::move(vigenere_text_5),
          std::move(vigenere_text_10),
          std::move(affine_symbol_text),
          std::move(affine_bigram_text),
      });
    }
    spdlog::info("Generated {} texts", n);
  }

  spdlog::info("Total chunks: {}", chunks.size());

  return 0;
}

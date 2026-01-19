#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <sstream>

// Variant 4

#include "alphabet.h"
#include "statistics.h"
#include "text_processor.h"

constexpr uint32_t L_ARR[] = {10, 100, 1000, 10000};
constexpr uint32_t N_ARR[] = {10000, 10000, 10000, 1000};

struct Chunk {
  std::unique_ptr<uint8_t[]> data;
  uint32_t len;
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

  for (int i = 0; i < 4; i++) {
    uint32_t l = L_ARR[i];
    uint32_t n = N_ARR[i];

    spdlog::info("L = {}", l);
    for (uint32_t j = 0; j < n; j++) {
      auto buff = std::make_unique<uint8_t[]>(static_cast<size_t>(l));

      std::copy(cursor, cursor + l, buff.get());
      cursor += l;

      chunks.push_back({std::move(buff), l});
    }
    spdlog::info("Generated {} texts", n);
  }

  return 0;
}

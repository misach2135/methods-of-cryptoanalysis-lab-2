#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <thread>
#include <tuple>
#include <unordered_set>

// Variant 4

#include "alphabet.h"
#include "criteria.h"
#include "statistics.h"
#include "text_processor.h"
#include "text_transformations.h"
#include "utils.h"

constexpr uint32_t L_ARR[] = {10, 100, 1000};
constexpr uint32_t N_ARR[] = {10000, 10000, 10000};

struct Loggers {
  std::shared_ptr<spdlog::logger> cli;
  std::shared_ptr<spdlog::logger> csv;
};

Loggers setupLogger();

int lab(const std::string& filepath, Loggers logs);

int main(int argc, char* argv[]) {
  if (argc != 2) {
    spdlog::error("Error: Path to the text is required.");
    return -1;
  }

  auto logs = setupLogger();

  auto res = lab(argv[1], std::move(logs));

  spdlog::shutdown();

  return res;
}

Loggers setupLogger() {
  constexpr std::size_t kQueueSize = 8192;
  constexpr std::size_t kThreads = 2;
  spdlog::init_thread_pool(kQueueSize, kThreads);

  auto cli_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  cli_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

  auto csv_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("results.csv", true);
  csv_sink->set_pattern("%v");

  auto cli = std::make_shared<spdlog::async_logger>(
      "cli", cli_sink, spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);
  auto csv = std::make_shared<spdlog::async_logger>(
      "csv", csv_sink, spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);

  cli->set_level(spdlog::level::info);
  csv->set_level(spdlog::level::info);

  spdlog::register_logger(cli);
  spdlog::register_logger(csv);

  bool write_header = true;
  {
    std::ifstream existing("results.csv", std::ios::binary | std::ios::ate);
    if (existing.is_open() && existing.tellg() > 0) {
      write_header = false;
    }
  }
  if (write_header) {
    csv->info("text_type,L,criterion,FP1,FN1,FP2,FN2");
  }

  return {cli, csv};
}

int lab(const std::string& filepath, Loggers logs) {
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

  logs.cli->info("Text is loaded. Text size in bytes: {}", text.size());

  logs.cli->info("Start preprocessing.");
  std::string processedText = lab2::prepareText(text);
  logs.cli->info("Preprocessing end. Text size after preprocessing: {}.",
                 processedText.size());

  // Since every letter is on cyrrylic page, we can use only the last bytes of
  // letters. This may reduce memory usage.
  logs.cli->info("Converting text to bytes.");
  auto bytes_vec = lab2::cyrillicTextToBytes(processedText);
  logs.cli->info("Bytes count(in practice -- letters count): {}.",
                 bytes_vec.size());

  logs.cli->info("Calculating statistics.");
  auto statistics = lab2::calculateStatistics(bytes_vec);

  logs.cli->info("Calculated statistics of preprocessed text:\n{}", statistics);

  logs.cli->info("Calculating forbidden symbols/bigrams for criteria.");

  std::unordered_set<uint8_t> forbidden_symbols;
  std::unordered_set<uint16_t> forbidden_bigrams;

  lab2::calculateForbiddenSymbols(forbidden_symbols, statistics.counts,
                                  statistics.text_size / 25);

  lab2::calculateForbiddenBigrams(forbidden_bigrams,
                                  statistics.overlapped_bigrams_count, 1);

  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)> plain_texts;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      vigenere_texts_1;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      vigenere_texts_5;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      vigenere_texts_10;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      affine_symbol_texts;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      affine_bigram_texts;

  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_plain_texts;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_vigenere_texts_1;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_vigenere_texts_5;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_vigenere_texts_10;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_affine_symbol_texts;
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_affine_bigram_texts;

  auto cursor = bytes_vec.begin();

  logs.cli->info("Start creating chunks");

  for (std::size_t i = 0; i < std::size(L_ARR); i++) {
    uint32_t l = L_ARR[i];
    uint32_t n = N_ARR[i];

    logs.cli->info("L = {}", l);
    if (l == 0U) {
      logs.cli->error("L is zero. Stopping chunk generation.");
      break;
    }
    std::size_t remaining =
        static_cast<std::size_t>(std::distance(cursor, bytes_vec.end()));
    logs.cli->info("Remaining chinks: {}", remaining);
    std::size_t max_chunks = remaining / l;
    if (max_chunks == 0) {
      logs.cli->warn("Not enough data for L = {}. Stopping chunk generation.",
                     l);
      break;
    }
    if (max_chunks < n) {
      logs.cli->warn("Requested {} chunks for L = {}, but only {} available.",
                     n, l, max_chunks);
      n = static_cast<uint32_t>(max_chunks);
    }

    for (uint32_t j = 0; j < n; j++) {
      std::vector<uint8_t> plain_text(l);

      std::copy(cursor, cursor + l, plain_text.data());
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
      std::vector<uint8_t> compressed_plain_text;
      std::vector<uint8_t> compressed_vigenere_text_1;
      std::vector<uint8_t> compressed_vigenere_text_5;
      std::vector<uint8_t> compressed_vigenere_text_10;
      std::vector<uint8_t> compressed_affine_symbol_text;
      std::vector<uint8_t> compressed_affine_bigram_text;

      auto t1 = std::thread([&vigenere_text_1, &plain_text, &vigenere_key_1,
                             &compressed_vigenere_text_1, &vigenere_texts_1,
                             &compressed_vigenere_texts_1, i]() {
        vigenere_text_1 = lab2::applyVigenereCipher(plain_text, vigenere_key_1);
        compressed_vigenere_text_1 = compressText(vigenere_text_1);
        vigenere_texts_1[i].push_back(vigenere_text_1);
        compressed_vigenere_texts_1[i].push_back(compressed_vigenere_text_1);
      });

      auto t2 = std::thread([&vigenere_text_5, &plain_text, &vigenere_key_5,
                             &compressed_vigenere_text_5, &vigenere_texts_5,
                             &compressed_vigenere_texts_5, i]() {
        vigenere_text_5 = lab2::applyVigenereCipher(plain_text, vigenere_key_5);
        compressed_vigenere_text_5 = compressText(vigenere_text_5);
        vigenere_texts_5[i].push_back(vigenere_text_5);
        compressed_vigenere_texts_5[i].push_back(compressed_vigenere_text_5);
      });

      auto t3 = std::thread([&vigenere_text_10, &plain_text, &vigenere_key_10,
                             &compressed_vigenere_text_10, &vigenere_texts_10,
                             &compressed_vigenere_texts_10, i]() {
        vigenere_text_10 =
            lab2::applyVigenereCipher(plain_text, vigenere_key_10);
        compressed_vigenere_text_10 = compressText(vigenere_text_10);
        vigenere_texts_10[i].push_back(vigenere_text_10);
        compressed_vigenere_texts_10[i].push_back(compressed_vigenere_text_10);
      });

      auto t4 = std::thread(
          [&affine_symbol_text, &plain_text, &compressed_affine_symbol_text, a1,
           b1, &affine_symbol_texts, &compressed_affine_symbol_texts, i]() {
            affine_symbol_text =
                lab2::applyAphineLetterSubstitution(plain_text, a1, b1);
            compressed_affine_symbol_text = compressText(affine_symbol_text);
            affine_symbol_texts[i].push_back(affine_symbol_text);
            compressed_affine_symbol_texts[i].push_back(
                compressed_affine_symbol_text);
          });

      auto t5 = std::thread(
          [&affine_bigram_text, &plain_text, &compressed_affine_bigram_text, a2,
           b2, &affine_bigram_texts, &compressed_affine_bigram_texts, i]() {
            affine_bigram_text =
                lab2::applyAphineBigramSubstitution(plain_text, a2, b2);
            compressed_affine_bigram_text = compressText(affine_bigram_text);
            affine_bigram_texts[i].push_back(affine_bigram_text);
            compressed_affine_bigram_texts[i].push_back(
                compressed_affine_bigram_text);
          });

      compressed_plain_text = compressText(plain_text);
      plain_texts[i].push_back(plain_text);
      compressed_plain_texts[i].push_back(compressed_plain_text);

      t1.join();
      t2.join();
      t3.join();
      t4.join();
      t5.join();
    }
    logs.cli->info("Generated {} texts", n);
  }

  return 0;
}

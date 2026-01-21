#include <spdlog/async.h>
#include <spdlog/fmt/fmt.h>
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
#include "interfaces.h"
#include "statistics.h"
#include "text_processor.h"
#include "text_transformations.h"
#include "toml.hpp"
#include "utils.h"

constexpr uint32_t L_ARR[] = {10, 100, 1000, 10000};
constexpr uint32_t N_ARR[] = {10000, 10000, 10000, 1000};

struct Config {
  double c11l1_threshold;
  double c30l1_threshold;
  double c51l1_j;
  double c51l1_threshold;

  double c11l2_threshold;
  double c30l2_threshold;
  double c51l2_j;
  double c51l2_threshold;

  double c_structural_threshold;

  double forbidden_symbols;
  double forbidden_bigrams;
};

struct Loggers {
  std::shared_ptr<spdlog::logger> cli;
  std::shared_ptr<spdlog::logger> crtireia_stats_csv;
  std::shared_ptr<spdlog::logger> criteria_structural_csv;
};

struct SerializedResult {
  uint32_t l;  // Text width
  uint32_t lgramSize;
  std::string criteria_identifier;
  std::string text_identifier;
  uint32_t h0;
  uint32_t h1;
  double fp;
  double fn;
};

template <>
struct fmt::formatter<SerializedResult> : fmt::formatter<std::string> {
  auto format(SerializedResult results, fmt::format_context& ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{},{},{},{},{},{},{},{}", results.l,
                          results.lgramSize, results.criteria_identifier,
                          results.text_identifier, results.h0, results.h1,
                          results.fp, results.fn);
  }
};

Loggers setupLogger();

Config loadConfig(const std::string& filename);

int lab(const std::string& filepath, Config config, Loggers logs);

int main(int argc, char* argv[]) {
  if (argc != 3) {
    spdlog::error("Error: Some argument is missing.");
    return -1;
  }

  auto logs = setupLogger();
  auto config = loadConfig(argv[1]);

  auto res = lab(argv[2], std::move(config), std::move(logs));

  spdlog::shutdown();

  return res;
}

Loggers setupLogger() {
  constexpr std::size_t kQueueSize = 8192;
  constexpr std::size_t kThreads = 2;
  spdlog::init_thread_pool(kQueueSize, kThreads);

  auto cli_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  cli_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

  auto criteria_csv_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "criteria_stats.csv", true);
  criteria_csv_sink->set_pattern("%v");

  auto structural_csv_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(
          "criteria_structural.csv", true);
  structural_csv_sink->set_pattern("%v");

  auto cli = std::make_shared<spdlog::async_logger>(
      "cli", cli_sink, spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);
  auto criteria_csv = std::make_shared<spdlog::async_logger>(
      "criteria_csv", criteria_csv_sink, spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);
  auto structural_csv = std::make_shared<spdlog::async_logger>(
      "structural_csv", structural_csv_sink, spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);

  cli->set_level(spdlog::level::info);
  criteria_csv->set_level(spdlog::level::info);
  structural_csv->set_level(spdlog::level::info);

  spdlog::register_logger(cli);
  spdlog::register_logger(criteria_csv);

  bool write_header = true;
  {
    std::ifstream existing("criteria_stats.csv",
                           std::ios::binary | std::ios::ate);
    if (existing.is_open() && existing.tellg() > 0) {
      write_header = false;
    }
  }
  if (write_header) {
    criteria_csv->info("l,lgramSize,criteria_id,text_id,h0,h1,fp,fn");
    structural_csv->info("text_id,l,abg_bits_per_byte");
  }

  return {cli, criteria_csv, structural_csv};
}

Config loadConfig(const std::string& filename) {
  auto config = toml::parse_file(filename);

  return Config{
      config["criteria_thresholds"]["c11l1_threshold"].value_or(0.0),
      config["criteria_thresholds"]["c30l1_threshold"].value_or(0.0),
      config["criteria_thresholds"]["c51l1_j"].value_or(0.0),
      config["criteria_thresholds"]["c51l1_threshold"].value_or(0.0),
      config["criteria_thresholds"]["c11l2_threshold"].value_or(0.0),
      config["criteria_thresholds"]["c30l2_threshold"].value_or(0.0),
      config["criteria_thresholds"]["c51l2_j"].value_or(0.0),
      config["criteria_thresholds"]["c51l2_threshold"].value_or(0.0),
      config["criteria_thresholds"]["c_structural_threshold"].value_or(0.0),
      config["forbidden_lgrams"]["forbidden_symbols"].value_or(0.0),
      config["forbidden_lgrams"]["forbidden_bigrams"].value_or(0.0),
  };
}

int lab(const std::string& filepath, Config config, Loggers logs) {
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

  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)> random_texts;

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
  std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>
      compressed_random_texts;

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
      std::vector<uint8_t> random_text;
      std::vector<uint8_t> compressed_plain_text;
      std::vector<uint8_t> compressed_vigenere_text_1;
      std::vector<uint8_t> compressed_vigenere_text_5;
      std::vector<uint8_t> compressed_vigenere_text_10;
      std::vector<uint8_t> compressed_affine_symbol_text;
      std::vector<uint8_t> compressed_affine_bigram_text;
      std::vector<uint8_t> compressed_random_text;

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

      auto t6 = std::thread([&random_text, &compressed_random_text,
                             &random_texts, &compressed_random_texts, l, i]() {
        random_text = lab2::generateRandomText(static_cast<size_t>(l));
        compressed_random_text = compressText(random_text);
        random_texts[i].push_back(random_text);
        compressed_random_texts[i].push_back(compressed_random_text);
      });

      compressed_plain_text = compressText(plain_text);
      plain_texts[i].push_back(plain_text);
      compressed_plain_texts[i].push_back(compressed_plain_text);

      t1.join();
      t2.join();
      t3.join();
      t4.join();
      t5.join();
      t6.join();
    }
    logs.cli->info("Generated {} texts", n);
  }

  logs.cli->info("Calculating forbidden symbols/bigrams for criteria.");

  std::unordered_set<uint8_t> forbidden_symbols;
  std::unordered_set<uint16_t> forbidden_bigrams;

  lab2::calculateForbiddenSymbols(
      forbidden_symbols, statistics.counts,
      statistics.text_size * config.forbidden_symbols);

  lab2::calculateForbiddenBigrams(
      forbidden_bigrams, statistics.overlapped_bigrams_count,
      (statistics.text_size - 1) * config.forbidden_bigrams);

  logs.cli->info("Forbidden Symbols: {}", forbidden_symbols);
  logs.cli->info("Forbidden Bigrams: {}", forbidden_bigrams);

  logs.cli->info("Calculating criterias...");

  std::vector<std::thread> threadPool;

  constexpr std::size_t criteria_count = 12U;
  constexpr std::size_t text_count = 14U;
  threadPool.reserve(plain_texts.size() * criteria_count * text_count);

  auto log_results = [&](std::size_t idx, uint32_t lgram_size,
                         const char* criteria_id, const char* text_id,
                         const CriteriaResult& res) {
    auto serialized_results = SerializedResult{
        L_ARR[idx],
        lgram_size,
        criteria_id,
        text_id,
        res.h0_count,
        res.h1_count,
        res.h0_count / static_cast<double>(N_ARR[idx]),
        res.h1_count / static_cast<double>(N_ARR[idx]),
    };
    logs.crtireia_stats_csv->info("{}", serialized_results);
  };

  auto enqueue_criteria_for_texts =
      [&](const std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>&
              texts,
          const std::array<std::vector<std::vector<uint8_t>>, std::size(L_ARR)>&
              compressed_texts,
          const char* text_id) {
        for (std::size_t i = 0; i < texts.size(); i++) {
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applySymbolCriteria10(texts[i], forbidden_symbols);
            log_results(i, 1U, "c10sy", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applySymbolCriteria11(texts[i], forbidden_symbols,
                                             config.c11l1_threshold * L_ARR[i]);
            log_results(i, 1U, "c11sy", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res =
                applySymbolCriteria12(texts[i], forbidden_symbols, statistics);
            log_results(i, 1U, "c12sy", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res =
                applySymbolCriteria13(texts[i], forbidden_symbols, statistics);
            log_results(i, 1U, "c13sy", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applySymbolCriteria30(texts[i], statistics,
                                             config.c30l1_threshold);
            log_results(i, 1U, "c30sy", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applySymbolCriteria51(
                texts[i], statistics, config.c51l1_threshold, config.c51l1_j);
            log_results(i, 1U, "c51sy", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applyBigramCriteria10(texts[i], forbidden_bigrams);
            log_results(i, 2U, "c10bi", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res =
                applyBigramCriteria11(texts[i], forbidden_bigrams,
                                      config.c11l2_threshold * (L_ARR[i] - 1));
            log_results(i, 2U, "c11bi", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res =
                applyBigramsCriteria12(texts[i], forbidden_bigrams, statistics);
            log_results(i, 2U, "c12bi", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res =
                applyBigramCriteria13(texts[i], forbidden_bigrams, statistics);
            log_results(i, 2U, "c13bi", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applyBigramCriteria30(texts[i], statistics,
                                             config.c30l2_threshold);
            log_results(i, 2U, "c30bi", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            auto res = applyBigramCriteria51(
                texts[i], statistics, config.c51l2_threshold, config.c51l2_j);
            log_results(i, 2U, "c51bi", text_id, res);
          });
          threadPool.emplace_back([&, i, text_id]() {
            double avg_bits_per_byte = 0.0;
            for (size_t j = 0; j < compressed_texts[i].size(); j++) {
              avg_bits_per_byte += lab2::getBitsPerSymbol(
                  texts[i][j].size(), compressed_texts[i][j].size());
            }
            avg_bits_per_byte /= compressed_texts[i].size();

            logs.cli->info(
                "Avg bits per byte for text of type {} of size {} : {}",
                text_id, L_ARR[i], avg_bits_per_byte);
            logs.criteria_structural_csv->info("{},{},{}", text_id, L_ARR[i],
                                               avg_bits_per_byte);
          });
        }
      };
  enqueue_criteria_for_texts(plain_texts, compressed_plain_texts, "plain");
  enqueue_criteria_for_texts(vigenere_texts_1, compressed_vigenere_texts_1,
                             "vig1");
  enqueue_criteria_for_texts(vigenere_texts_5, compressed_vigenere_texts_5,
                             "vig5");
  enqueue_criteria_for_texts(vigenere_texts_10, compressed_vigenere_texts_10,
                             "vig10");
  enqueue_criteria_for_texts(affine_symbol_texts,
                             compressed_affine_symbol_texts, "affine_sym");
  enqueue_criteria_for_texts(affine_bigram_texts,
                             compressed_affine_bigram_texts, "affine_bi");
  enqueue_criteria_for_texts(random_texts, compressed_random_texts, "random");

  for (auto& worker : threadPool) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  return 0;
}

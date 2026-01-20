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

constexpr uint32_t L_ARR[] = {10, 100, 1000};
constexpr uint32_t N_ARR[] = {10000, 10000, 10000};

constexpr uint32_t BIGRAM_THRESHOLDS[] = {1, 2, 3};
constexpr uint32_t SYMBOL_THRESHOLDS[] = {3, 4, 5};

struct Loggers {
  std::shared_ptr<spdlog::logger> cli;
  std::shared_ptr<spdlog::logger> csv;
};

struct Chunk {
  uint32_t l;
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

struct CriterionSpec {
  std::string name;
  uint8_t l;
  std::function<bool(const std::vector<uint8_t>&)> apply;
};

struct ResultKey {
  std::string config;
  uint32_t L;
  std::string criterion;
  uint8_t l;

  bool operator<(const ResultKey& other) const {
    return std::tie(config, L, criterion, l) <
           std::tie(other.config, other.L, other.criterion, other.l);
  }
};

struct ResultCounts {
  uint64_t fp = 0;
  uint64_t fn = 0;
  uint64_t n_h0 = 0;
  uint64_t n_h1 = 0;
};

Loggers setupLogger();

int lab(const std::string& filepath, Loggers logs);

int main(int argc, char* argv[]) {
  if (argc != 2) {
    spdlog::error("Error: Path to the text is required.");
    return -1;
  }

  auto logs = setupLogger();

  auto res = lab(argv[1], logs);

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
    csv->info("config,L,criterion,l,FP,FN,N_H0,N_H1");
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
  std::vector<
      std::pair<uint32_t, std::shared_ptr<const std::unordered_set<uint8_t>>>>
      symbol_forbidden_sets;
  symbol_forbidden_sets.reserve(std::size(SYMBOL_THRESHOLDS));
  for (auto threshold : SYMBOL_THRESHOLDS) {
    auto forbidden = std::make_shared<std::unordered_set<uint8_t>>();
    lab2::calculateForbiddenSymbols(*forbidden, statistics.counts, threshold);
    symbol_forbidden_sets.push_back({threshold, std::move(forbidden)});
  }

  std::vector<
      std::pair<uint32_t, std::shared_ptr<const std::unordered_set<uint16_t>>>>
      bigram_forbidden_sets;
  bigram_forbidden_sets.reserve(std::size(BIGRAM_THRESHOLDS));
  for (auto threshold : BIGRAM_THRESHOLDS) {
    auto forbidden = std::make_shared<std::unordered_set<uint16_t>>();
    lab2::calculateForbiddenBigrams(
        *forbidden, statistics.overlapped_bigrams_count, threshold);
    bigram_forbidden_sets.push_back({threshold, std::move(forbidden)});
  }

  std::vector<CriterionSpec> criteria;
  criteria.reserve(std::size(SYMBOL_THRESHOLDS) * 4U +
                   std::size(BIGRAM_THRESHOLDS) * 4U + 4U);

  for (const auto& entry : symbol_forbidden_sets) {
    const uint32_t threshold = entry.first;
    const auto& forbidden = entry.second;
    const std::string suffix = "_k" + std::to_string(threshold);

    criteria.push_back(
        {"sym_1.0" + suffix, 1, [forbidden](const std::vector<uint8_t>& text) {
           return lab2::symbolicCriteria10(text, *forbidden);
         }});
    criteria.push_back(
        {"sym_1.1" + suffix, 1,
         [forbidden, threshold](const std::vector<uint8_t>& text) {
           return lab2::symbolicCriteria11(text, *forbidden, threshold);
         }});
    criteria.push_back(
        {"sym_1.2" + suffix, 1,
         [forbidden, &statistics](const std::vector<uint8_t>& text) {
           return lab2::symbolicCriteria12(text, *forbidden, statistics);
         }});
    criteria.push_back(
        {"sym_1.3" + suffix, 1,
         [forbidden, &statistics](const std::vector<uint8_t>& text) {
           return lab2::symbolicCriteria13(text, *forbidden, statistics);
         }});
  }

  criteria.push_back(
      {"sym_3.0", 1, [&statistics](const std::vector<uint8_t>& text) {
         return lab2::symbolicCriteria30(text, statistics);
       }});
  criteria.push_back(
      {"sym_5.1", 1, [&statistics](const std::vector<uint8_t>& text) {
         return lab2::symbolicCriteria51(text, statistics);
       }});

  for (const auto& entry : bigram_forbidden_sets) {
    const uint32_t threshold = entry.first;
    const auto& forbidden = entry.second;
    const std::string suffix = "_k" + std::to_string(threshold);

    criteria.push_back(
        {"big_1.0" + suffix, 2, [forbidden](const std::vector<uint8_t>& text) {
           return lab2::bigramCriteria10(text, *forbidden);
         }});
    criteria.push_back(
        {"big_1.1" + suffix, 2,
         [forbidden, threshold](const std::vector<uint8_t>& text) {
           return lab2::bigramCriteria11(text, *forbidden, threshold);
         }});
    criteria.push_back(
        {"big_1.2" + suffix, 2,
         [forbidden, &statistics](const std::vector<uint8_t>& text) {
           return lab2::bigramCriteria12(text, *forbidden, statistics);
         }});
    criteria.push_back(
        {"big_1.3" + suffix, 2,
         [forbidden, &statistics](const std::vector<uint8_t>& text) {
           return lab2::bigramCriteria13(text, *forbidden, statistics);
         }});
  }

  criteria.push_back(
      {"big_3.0", 2, [&statistics](const std::vector<uint8_t>& text) {
         return lab2::bigramCriteria30(text, statistics);
       }});
  criteria.push_back(
      {"big_5.1", 2, [&statistics](const std::vector<uint8_t>& text) {
         return lab2::bigramCriteria51(text, statistics);
       }});
  logs.cli->info("Prepared {} criteria.", criteria.size());

  std::vector<Chunk> chunks;
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
          l,
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
    logs.cli->info("Generated {} texts", n);
  }

  logs.cli->info("Total chunks: {}", chunks.size());

  logs.cli->info("criteria_return_true_means=H1");
  logs.cli->info("Applying criterias to chunks...");

  const std::array<std::string, 5> config_names = {
      "vigenere_r1",   "vigenere_r5",   "vigenere_r10",
      "affine_symbol", "affine_bigram",
  };

  std::map<ResultKey, ResultCounts> results;
  std::vector<uint8_t> h0_results(criteria.size());

  for (const auto& chunk : chunks) {
    const std::array<const std::vector<uint8_t>*, 5> h1_texts = {
        &chunk.vigenere_text_1,    &chunk.vigenere_text_5,
        &chunk.vigenere_text_10,   &chunk.affine_symbol_text,
        &chunk.affine_bigram_text,
    };

    const auto& h0_text = chunk.plain_text;
    for (std::size_t c = 0; c < criteria.size(); ++c) {
      h0_results[c] = criteria[c].apply(h0_text) ? 1U : 0U;
    }

    for (std::size_t i = 0; i < config_names.size(); ++i) {
      const auto& config = config_names[i];
      const auto& h1_text = *h1_texts[i];

      for (std::size_t c = 0; c < criteria.size(); ++c) {
        const auto& criterion = criteria[c];
        const bool h0_is_h1 = h0_results[c] != 0U;
        const bool h1_is_h1 = criterion.apply(h1_text);

        ResultKey key{config, chunk.l, criterion.name, criterion.l};
        auto& counts = results[key];
        ++counts.n_h0;
        ++counts.n_h1;
        if (h0_is_h1) {
          ++counts.fp;
        }
        if (!h1_is_h1) {
          ++counts.fn;
        }
      }
    }
  }

  logs.cli->info("Writing CSV results.");
  for (const auto& [key, counts] : results) {
    const double fp =
        (counts.n_h0 == 0U)
            ? 0.0
            : static_cast<double>(counts.fp) / static_cast<double>(counts.n_h0);
    const double fn =
        (counts.n_h1 == 0U)
            ? 0.0
            : static_cast<double>(counts.fn) / static_cast<double>(counts.n_h1);

    logs.csv->info("{},{},{},{},{:.6f},{:.6f},{},{}", key.config, key.L,
                   key.criterion, static_cast<uint32_t>(key.l), fp, fn,
                   counts.n_h0, counts.n_h1);
  }

  logs.cli->info("Summary (FP/FN per config and L):");
  std::string current_config;
  uint32_t current_L = 0U;
  bool first = true;
  for (const auto& [key, counts] : results) {
    const double fp =
        (counts.n_h0 == 0U)
            ? 0.0
            : static_cast<double>(counts.fp) / static_cast<double>(counts.n_h0);
    const double fn =
        (counts.n_h1 == 0U)
            ? 0.0
            : static_cast<double>(counts.fn) / static_cast<double>(counts.n_h1);

    if (first || key.config != current_config || key.L != current_L) {
      logs.cli->info("config={}, L={}", key.config, key.L);
      current_config = key.config;
      current_L = key.L;
      first = false;
    }

    logs.cli->info("  criterion={}, l={}, FP={:.4f}, FN={:.4f}", key.criterion,
                   static_cast<uint32_t>(key.l), fp, fn);
  }

  return 0;
}

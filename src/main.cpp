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
constexpr std::size_t kLCount = std::size(L_ARR);  // Per-L config sizing.

struct Config {
  std::array<double, kLCount> c11l1_threshold;  // Ratios, scaled by L.
  std::array<double, kLCount> c30l1_threshold;  // Entropy delta thresholds.
  std::array<std::size_t, kLCount> c51l1_j;     // Top-j parameter per L.
  std::array<std::size_t, kLCount> c51l1_threshold;  // Count threshold per L.

  std::array<double, kLCount> c11l2_threshold;  // Ratios, scaled by L-1.
  std::array<double, kLCount> c30l2_threshold;  // Entropy delta thresholds.
  std::array<std::size_t, kLCount> c51l2_j;     // Top-j parameter per L.
  std::array<std::size_t, kLCount> c51l2_threshold;  // Count threshold per L.

  std::array<double, kLCount> c_structural_threshold;  // Per-L structural thresholds.

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
  double threshold;  // Threshold used for the current L.
  std::size_t j;     // Additional parameter for c51 criteria.
};

template <>
struct fmt::formatter<SerializedResult> : fmt::formatter<std::string> {
  auto format(SerializedResult results, fmt::format_context& ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(
        ctx.out(), "{},{},{},{},{},{},{},{},{},{}", results.l,
        results.lgramSize, results.criteria_identifier, results.text_identifier,
        results.h0, results.h1, results.fp, results.fn, results.threshold,
        results.j);
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
    // Add threshold fields to CSV headers for per-L configs.
    criteria_csv->info(
        "l,lgramSize,criteria_id,text_id,h0,h1,fp,fn,threshold,j");
    structural_csv->info("text_id,l,abg_bits_per_byte,threshold");
  }

  return {cli, criteria_csv, structural_csv};
}

// Load per-L values from a scalar or an array (arrays follow L_ARR order).
std::array<double, kLCount> loadPerLDouble(
    const toml::node_view<toml::node>& node, double default_value) {
  std::array<double, kLCount> values{};
  values.fill(default_value);

  if (!node) {
    return values;
  }

  if (node.is_array()) {
    auto* arr = node.as_array();
    const auto count = arr->size() < kLCount ? arr->size() : kLCount;
    for (std::size_t i = 0; i < count; ++i) {
      auto* elem = arr->get(i);
      if (!elem) {
        continue;
      }
      if (auto val = elem->value<double>(); val) {
        values[i] = *val;
        continue;
      }
      if (auto val_int = elem->value<int64_t>(); val_int) {
        values[i] = static_cast<double>(*val_int);
      }
    }
    return values;
  }

  if (auto val = node.value<double>(); val) {
    values.fill(*val);
  } else if (auto val_int = node.value<int64_t>(); val_int) {
    values.fill(static_cast<double>(*val_int));
  }
  return values;
}

// Load per-L integer values from a scalar or an array.
std::array<std::size_t, kLCount> loadPerLSize(
    const toml::node_view<toml::node>& node, std::size_t default_value) {
  std::array<std::size_t, kLCount> values{};
  values.fill(default_value);

  if (!node) {
    return values;
  }

  if (node.is_array()) {
    auto* arr = node.as_array();
    const auto count = arr->size() < kLCount ? arr->size() : kLCount;
    for (std::size_t i = 0; i < count; ++i) {
      auto* elem = arr->get(i);
      if (!elem) {
        continue;
      }
      if (auto val = elem->value<int64_t>(); val && *val >= 0) {
        values[i] = static_cast<std::size_t>(*val);
      }
    }
    return values;
  }

  if (auto val = node.value<int64_t>(); val && *val >= 0) {
    values.fill(static_cast<std::size_t>(*val));
  }
  return values;
}

Config loadConfig(const std::string& filename) {
  auto config = toml::parse_file(filename);

  // Allow scalar or per-L arrays for all thresholds.
  const auto thresholds = config["criteria_thresholds"];

  return Config{
      loadPerLDouble(thresholds["c11l1_threshold"], 0.0),
      loadPerLDouble(thresholds["c30l1_threshold"], 0.0),
      loadPerLSize(thresholds["c51l1_j"], 0U),
      loadPerLSize(thresholds["c51l1_threshold"], 0U),
      loadPerLDouble(thresholds["c11l2_threshold"], 0.0),
      loadPerLDouble(thresholds["c30l2_threshold"], 0.0),
      loadPerLSize(thresholds["c51l2_j"], 0U),
      loadPerLSize(thresholds["c51l2_threshold"], 0U),
      loadPerLDouble(thresholds["c_structural_threshold"], 0.0),
      config["forbidden_lgrams"]["forbidden_symbols"].value_or(0.0),
      config["forbidden_lgrams"]["forbidden_bigrams"].value_or(0.0),
  };
}

int lab(const std::string& filepath, Config config, Loggers logs) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> uniform_distribution(
      0, ALPHABET_SIZE - 1U);

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

  lab2::calculateForbiddenSymbols(forbidden_symbols, statistics.counts,
                                  config.forbidden_symbols,
                                  statistics.text_size);

  lab2::calculateForbiddenBigrams(
      forbidden_bigrams, statistics.overlapped_bigrams_count,
      config.forbidden_bigrams, statistics.text_size - 1);

  logs.cli->info("Forbidden Symbols: {}", forbidden_symbols);
  logs.cli->info("Forbidden Bigrams: {}", forbidden_bigrams);

  logs.cli->info("Calculating criterias...");

  constexpr std::size_t kCriteriaCount = 12U;

  struct CriteriaMeta {
    const char* id;
    uint32_t lgram_size;
  };

  constexpr std::array<CriteriaMeta, kCriteriaCount> kCriteriaMeta = {{
      {"c10sy", 1U},
      {"c11sy", 1U},
      {"c12sy", 1U},
      {"c13sy", 1U},
      {"c30sy", 1U},
      {"c51sy", 1U},
      {"c10bi", 2U},
      {"c11bi", 2U},
      {"c12bi", 2U},
      {"c13bi", 2U},
      {"c30bi", 2U},
      {"c51bi", 2U},
  }};

  // Convert ratio thresholds to absolute counts per L for c11 criteria.
  std::array<std::size_t, kLCount> c11l1_threshold_by_l{};
  std::array<std::size_t, kLCount> c11l2_threshold_by_l{};
  for (std::size_t i = 0; i < kLCount; ++i) {
    c11l1_threshold_by_l[i] =
        static_cast<std::size_t>(config.c11l1_threshold[i] * L_ARR[i]);
    c11l2_threshold_by_l[i] = static_cast<std::size_t>(
        config.c11l2_threshold[i] * (L_ARR[i] - 1U));
  }

  using CriteriaMatrix =
      std::array<std::array<CriteriaResult, kCriteriaCount>, kLCount>;

  CriteriaMatrix plain_results{};
  CriteriaMatrix vig1_results{};
  CriteriaMatrix vig5_results{};
  CriteriaMatrix vig10_results{};
  CriteriaMatrix affine_sym_results{};
  CriteriaMatrix affine_bi_results{};
  CriteriaMatrix random_results{};

  std::vector<std::thread> threadPool;

  constexpr std::size_t text_type_count = 7U;
  threadPool.reserve(kLCount * text_type_count);

  auto enqueue_criteria_for_texts = [&](const auto& texts,
                                        CriteriaMatrix& results) {
    auto* texts_ptr = &texts;
    auto* results_ptr = &results;
    for (std::size_t i = 0; i < texts.size(); i++) {
      threadPool.emplace_back([&, i, texts_ptr, results_ptr]() {
        (*results_ptr)[i][0] =
            applySymbolCriteria10((*texts_ptr)[i], forbidden_symbols);
        (*results_ptr)[i][1] =
            applySymbolCriteria11((*texts_ptr)[i], forbidden_symbols,
                                  c11l1_threshold_by_l[i]);
        (*results_ptr)[i][2] = applySymbolCriteria12(
            (*texts_ptr)[i], forbidden_symbols, statistics);
        (*results_ptr)[i][3] = applySymbolCriteria13(
            (*texts_ptr)[i], forbidden_symbols, statistics);
        (*results_ptr)[i][4] = applySymbolCriteria30(
            (*texts_ptr)[i], statistics, config.c30l1_threshold[i]);
        (*results_ptr)[i][5] =
            applySymbolCriteria51((*texts_ptr)[i], statistics,
                                  config.c51l1_threshold[i],
                                  config.c51l1_j[i]);
        (*results_ptr)[i][6] =
            applyBigramCriteria10((*texts_ptr)[i], forbidden_bigrams);
        (*results_ptr)[i][7] =
            applyBigramCriteria11((*texts_ptr)[i], forbidden_bigrams,
                                  c11l2_threshold_by_l[i]);
        (*results_ptr)[i][8] = applyBigramsCriteria12(
            (*texts_ptr)[i], forbidden_bigrams, statistics);
        (*results_ptr)[i][9] = applyBigramCriteria13(
            (*texts_ptr)[i], forbidden_bigrams, statistics);
        (*results_ptr)[i][10] = applyBigramCriteria30(
            (*texts_ptr)[i], statistics, config.c30l2_threshold[i]);
        (*results_ptr)[i][11] =
            applyBigramCriteria51((*texts_ptr)[i], statistics,
                                  config.c51l2_threshold[i],
                                  config.c51l2_j[i]);
      });
    }
  };

  enqueue_criteria_for_texts(plain_texts, plain_results);
  enqueue_criteria_for_texts(vigenere_texts_1, vig1_results);
  enqueue_criteria_for_texts(vigenere_texts_5, vig5_results);
  enqueue_criteria_for_texts(vigenere_texts_10, vig10_results);
  enqueue_criteria_for_texts(affine_symbol_texts, affine_sym_results);
  enqueue_criteria_for_texts(affine_bigram_texts, affine_bi_results);
  enqueue_criteria_for_texts(random_texts, random_results);

  for (auto& worker : threadPool) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  auto count_to_rate = [](uint32_t count, std::size_t total) -> double {
    if (total == 0U) {
      return 0.0;
    }
    return static_cast<double>(count) / static_cast<double>(total);
  };

  std::array<std::array<double, kCriteriaCount>, kLCount> plain_fp{};
  for (std::size_t i = 0; i < kLCount; i++) {
    const auto total = plain_texts[i].size();
    for (std::size_t c = 0; c < kCriteriaCount; c++) {
      plain_fp[i][c] = count_to_rate(plain_results[i][c].h1_count, total);
    }
  }

  // Map per-L thresholds to each criteria for logging.
  auto thresholds_for_log =
      [&](std::size_t idx,
          std::size_t criteria_idx) -> std::pair<double, std::size_t> {
    switch (criteria_idx) {
      case 1:  // c11sy
        return {static_cast<double>(c11l1_threshold_by_l[idx]), 0U};
      case 4:  // c30sy
        return {config.c30l1_threshold[idx], 0U};
      case 5:  // c51sy
        return {static_cast<double>(config.c51l1_threshold[idx]),
                config.c51l1_j[idx]};
      case 7:  // c11bi
        return {static_cast<double>(c11l2_threshold_by_l[idx]), 0U};
      case 10:  // c30bi
        return {config.c30l2_threshold[idx], 0U};
      case 11:  // c51bi
        return {static_cast<double>(config.c51l2_threshold[idx]),
                config.c51l2_j[idx]};
      default:
        return {0.0, 0U};
    }
  };

  auto log_results = [&](std::size_t idx, std::size_t criteria_idx,
                         const char* text_id, const CriteriaResult& res,
                         double fp, double fn) {
    const auto& meta = kCriteriaMeta[criteria_idx];
    const auto [threshold, j] = thresholds_for_log(idx, criteria_idx);
    auto serialized_results = SerializedResult{
        L_ARR[idx],   meta.lgram_size, meta.id, text_id,
        res.h0_count, res.h1_count,    fp,      fn,
        threshold,    j,
    };
    logs.crtireia_stats_csv->info("{}", serialized_results);
  };

  auto log_for_texts = [&](const char* text_id, const auto& texts,
                           const CriteriaMatrix& results, bool is_plain) {
    for (std::size_t i = 0; i < texts.size(); i++) {
      const auto total = texts[i].size();
      for (std::size_t c = 0; c < kCriteriaCount; c++) {
        // False positives are counted on plaintexts; false negatives on ciphertexts.
        const double fp = plain_fp[i][c];
        const double fn =
            is_plain ? 0.0 : count_to_rate(results[i][c].h0_count, total);
        log_results(i, c, text_id, results[i][c], fp, fn);
      }
    }
  };

  log_for_texts("plain", plain_texts, plain_results, true);
  log_for_texts("vig1", vigenere_texts_1, vig1_results, false);
  log_for_texts("vig5", vigenere_texts_5, vig5_results, false);
  log_for_texts("vig10", vigenere_texts_10, vig10_results, false);
  log_for_texts("affine_sym", affine_symbol_texts, affine_sym_results, false);
  log_for_texts("affine_bi", affine_bigram_texts, affine_bi_results, false);
  log_for_texts("random", random_texts, random_results, false);

  auto log_structural_stats = [&](const char* text_id, const auto& texts,
                                  const auto& compressed_texts) {
    for (std::size_t i = 0; i < texts.size(); i++) {
      double avg_bits_per_byte = 0.0;
      const auto total = compressed_texts[i].size();
      if (total != 0U) {
        for (std::size_t j = 0; j < total; j++) {
          avg_bits_per_byte += lab2::getBitsPerSymbol(
              texts[i][j].size(), compressed_texts[i][j].size());
        }
        avg_bits_per_byte /= static_cast<double>(total);
      }

      logs.cli->info("Avg bits per byte for text of type {} of size {} : {}",
                     text_id, L_ARR[i], avg_bits_per_byte);
      // Log structural thresholds alongside the averaged compression stats.
      logs.criteria_structural_csv->info(
          "{},{},{},{}", text_id, L_ARR[i], avg_bits_per_byte,
          config.c_structural_threshold[i]);
    }
  };

  log_structural_stats("plain", plain_texts, compressed_plain_texts);
  log_structural_stats("vig1", vigenere_texts_1, compressed_vigenere_texts_1);
  log_structural_stats("vig5", vigenere_texts_5, compressed_vigenere_texts_5);
  log_structural_stats("vig10", vigenere_texts_10,
                       compressed_vigenere_texts_10);
  log_structural_stats("affine_sym", affine_symbol_texts,
                       compressed_affine_symbol_texts);
  log_structural_stats("affine_bi", affine_bigram_texts,
                       compressed_affine_bigram_texts);
  log_structural_stats("random", random_texts, compressed_random_texts);

  return 0;
}

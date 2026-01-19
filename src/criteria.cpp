#include "criteria.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "alphabet.h"

namespace lab2 {

// Return value: true => H1 (random / non-plaintext), false => H0 (plaintext).
// Bigram criteria use overlapped bigrams encoded as a*ALPHABET_SIZE + b.
// Threshold defaults (tune for FP/FN): kKp1/kKp2 from criteria.h for 1.1,
// kEntropyThresholdSymbols/kEntropyThresholdBigrams for 3.0, and for 5.1
// use kFrequentSymbols/kFrequentBigrams (j) with kEmptySymbols/kEmptyBigrams.
namespace {

constexpr double kEntropyThresholdSymbols = 0.2;  // kH for l=1 (heuristic).
constexpr double kEntropyThresholdBigrams =
    0.3;  // kH for l=2 (looser due to higher variance).
constexpr std::size_t kFrequentSymbols = 10;   // j for 5.1 (symbols).
constexpr std::size_t kFrequentBigrams = 100;  // j for 5.1 (bigrams, lab uses
                                               // 50/100/200 -> choose 100).
constexpr std::size_t kEmptySymbols = 6;   // kempt for 5.1 (symbol, ~60% of j).
constexpr std::size_t kEmptyBigrams = 60;  // kempt for 5.1 (bigram, ~60% of j).

uint16_t makeBigram(uint8_t first, uint8_t second) {
  return static_cast<uint16_t>(first * ALPHABET_SIZE + second);
}

std::size_t totalOverlappedBigrams(std::size_t text_size) {
  return (text_size > 1U) ? (text_size - 1U) : 0U;
}

template <typename Counts>
double calculateEntropyFromCounts(const Counts& counts, double total,
                                  std::size_t gram_len) {
  if (total <= 0.0 || gram_len == 0U) {
    return 0.0;
  }

  double entropy = 0.0;
  for (const auto& [gram, count] : counts) {
    if (count == 0U) {
      continue;
    }
    double prob = static_cast<double>(count) / total;
    entropy += -prob * std::log2(prob);
  }

  return entropy / static_cast<double>(gram_len);
}

}  // namespace

bool symbolicCriteria10(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols) {
  if (text.empty()) {
    return true;
  }

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) != forbidden_symbols.end()) {
      return true;
    }
  }

  return false;
}

bool symbolicCriteria11(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        const Statistics& statistics) {
  if (text.empty()) {
    return true;
  }

  std::unordered_set<uint8_t> appeared;
  appeared.reserve(std::min<std::size_t>(forbidden_symbols.size(), kKp1));

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) != forbidden_symbols.end()) {
      appeared.insert(symbol);
      if (appeared.size() >= kKp1) {
        return true;
      }
    }
  }

  return false;
}

bool symbolicCriteria12(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        const Statistics& statistics) {
  if (text.empty()) {
    return true;
  }

  const double total = static_cast<double>(text.size());
  const double baseline_total = static_cast<double>(statistics.text_size);
  if (baseline_total <= 0.0) {
    return true;
  }

  std::unordered_map<uint8_t, uint32_t> sample_counts;
  sample_counts.reserve(forbidden_symbols.size());

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) != forbidden_symbols.end()) {
      ++sample_counts[symbol];
    }
  }

  for (auto symbol : forbidden_symbols) {
    auto sample_it = sample_counts.find(symbol);
    const uint32_t sample_count =
        (sample_it == sample_counts.end()) ? 0U : sample_it->second;
    const double fx = static_cast<double>(sample_count) / total;

    auto base_it = statistics.counts.find(symbol);
    const uint32_t base_count =
        (base_it == statistics.counts.end()) ? 0U : base_it->second;
    const double kx = static_cast<double>(base_count) / baseline_total;

    if (fx > kx) {
      return true;
    }
  }

  return false;
}

// Criterion 1.3 (sum of forbidden symbols frequencies).
bool symbolicCriteria13(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        const Statistics& statistics) {
  if (text.empty()) {
    return true;
  }

  const double total = static_cast<double>(text.size());
  const double baseline_total = static_cast<double>(statistics.text_size);
  if (baseline_total <= 0.0) {
    return true;
  }

  std::unordered_map<uint8_t, uint32_t> sample_counts;
  sample_counts.reserve(forbidden_symbols.size());

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) != forbidden_symbols.end()) {
      ++sample_counts[symbol];
    }
  }

  double fp_sum = 0.0;
  double kp_sum = 0.0;
  for (auto symbol : forbidden_symbols) {
    auto sample_it = sample_counts.find(symbol);
    const uint32_t sample_count =
        (sample_it == sample_counts.end()) ? 0U : sample_it->second;
    fp_sum += static_cast<double>(sample_count) / total;

    auto base_it = statistics.counts.find(symbol);
    const uint32_t base_count =
        (base_it == statistics.counts.end()) ? 0U : base_it->second;
    kp_sum += static_cast<double>(base_count) / baseline_total;
  }

  return fp_sum > kp_sum;
}

// Criterion 3.0 (entropy, symbols).
bool symbolicCriteria30(const std::vector<uint8_t>& text,
                        const Statistics& statistics) {
  if (text.empty() || statistics.text_size == 0U) {
    return true;
  }

  std::unordered_map<uint8_t, uint32_t> sample_counts;
  sample_counts.reserve(ALPHABET_SIZE);
  calculateLetterFrequencies(text, sample_counts);
  double sample_entropy = calculateEntropy(sample_counts, text.size());

  return std::abs(statistics.entropy - sample_entropy) >
         kEntropyThresholdSymbols;
}

// Criterion 5.1 (empty boxes, most frequent symbols).
bool symbolicCriteria51(const std::vector<uint8_t>& text,
                        const Statistics& statistics) {
  if (text.empty() || statistics.text_size == 0U) {
    return true;
  }

  std::vector<std::pair<uint8_t, uint32_t>> ordered;
  ordered.reserve(ALPHABET_SIZE);
  for (uint16_t c = 0; c < ALPHABET_SIZE; ++c) {
    const auto symbol = static_cast<uint8_t>(c);
    auto it = statistics.counts.find(symbol);
    const uint32_t count = (it == statistics.counts.end()) ? 0U : it->second;
    ordered.push_back({symbol, count});
  }

  const std::size_t top_count =
      std::min<std::size_t>(kFrequentSymbols, ordered.size());
  if (top_count == 0U) {
    return true;
  }

  std::partial_sort(
      ordered.begin(), ordered.begin() + top_count, ordered.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

  std::unordered_set<uint8_t> frequent_symbols;
  frequent_symbols.reserve(top_count * 2U);
  for (std::size_t i = 0; i < top_count; ++i) {
    frequent_symbols.insert(ordered[i].first);
  }

  std::unordered_set<uint8_t> seen;
  seen.reserve(top_count);
  for (auto symbol : text) {
    if (frequent_symbols.find(symbol) != frequent_symbols.end()) {
      seen.insert(symbol);
      if (seen.size() == frequent_symbols.size()) {
        break;
      }
    }
  }

  const std::size_t fempt = frequent_symbols.size() - seen.size();
  return fempt >= kEmptySymbols;
}

// Criterion 1.0 (forbidden bigrams, overlapped).
bool bigramCriteria10(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics) {
  (void)statistics;
  if (text.size() < 2U) {
    return true;
  }

  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    const uint16_t bigram = makeBigram(text[i], text[i + 1]);
    if (forbidden_symbols.find(bigram) != forbidden_symbols.end()) {
      return true;
    }
  }

  return false;
}

// Criterion 1.1 (forbidden bigrams with kp threshold, overlapped).
bool bigramCriteria11(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics) {
  (void)statistics;
  if (text.size() < 2U) {
    return true;
  }

  std::unordered_set<uint16_t> appeared;
  appeared.reserve(std::min<std::size_t>(forbidden_symbols.size(), kKp2));

  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    const uint16_t bigram = makeBigram(text[i], text[i + 1]);
    if (forbidden_symbols.find(bigram) != forbidden_symbols.end()) {
      appeared.insert(bigram);
      if (appeared.size() >= kKp2) {
        return true;
      }
    }
  }

  return false;
}

// Criterion 1.2 (forbidden bigram frequency vs baseline, overlapped).
bool bigramCriteria12(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics) {
  if (text.size() < 2U) {
    return true;
  }

  const std::size_t total = totalOverlappedBigrams(text.size());
  const std::size_t baseline_total =
      totalOverlappedBigrams(statistics.text_size);
  if (total == 0U || baseline_total == 0U) {
    return true;
  }

  std::unordered_map<uint16_t, uint32_t> sample_counts;
  sample_counts.reserve(forbidden_symbols.size());

  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    const uint16_t bigram = makeBigram(text[i], text[i + 1]);
    if (forbidden_symbols.find(bigram) != forbidden_symbols.end()) {
      ++sample_counts[bigram];
    }
  }

  const double total_d = static_cast<double>(total);
  const double baseline_total_d = static_cast<double>(baseline_total);
  for (auto bigram : forbidden_symbols) {
    auto sample_it = sample_counts.find(bigram);
    const uint32_t sample_count =
        (sample_it == sample_counts.end()) ? 0U : sample_it->second;
    const double fx = static_cast<double>(sample_count) / total_d;

    auto base_it = statistics.overlapped_bigrams_count.find(bigram);
    const uint32_t base_count =
        (base_it == statistics.overlapped_bigrams_count.end())
            ? 0U
            : base_it->second;
    const double kx = static_cast<double>(base_count) / baseline_total_d;

    if (fx > kx) {
      return true;
    }
  }

  return false;
}

// Criterion 1.3 (sum of forbidden bigram frequencies, overlapped).
bool bigramCriteria13(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics) {
  if (text.size() < 2U) {
    return true;
  }

  const std::size_t total = totalOverlappedBigrams(text.size());
  const std::size_t baseline_total =
      totalOverlappedBigrams(statistics.text_size);
  if (total == 0U || baseline_total == 0U) {
    return true;
  }

  std::unordered_map<uint16_t, uint32_t> sample_counts;
  sample_counts.reserve(forbidden_symbols.size());

  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    const uint16_t bigram = makeBigram(text[i], text[i + 1]);
    if (forbidden_symbols.find(bigram) != forbidden_symbols.end()) {
      ++sample_counts[bigram];
    }
  }

  double fp_sum = 0.0;
  double kp_sum = 0.0;
  const double total_d = static_cast<double>(total);
  const double baseline_total_d = static_cast<double>(baseline_total);
  for (auto bigram : forbidden_symbols) {
    auto sample_it = sample_counts.find(bigram);
    const uint32_t sample_count =
        (sample_it == sample_counts.end()) ? 0U : sample_it->second;
    fp_sum += static_cast<double>(sample_count) / total_d;

    auto base_it = statistics.overlapped_bigrams_count.find(bigram);
    const uint32_t base_count =
        (base_it == statistics.overlapped_bigrams_count.end())
            ? 0U
            : base_it->second;
    kp_sum += static_cast<double>(base_count) / baseline_total_d;
  }

  return fp_sum > kp_sum;
}

// Criterion 3.0 (entropy, bigrams, overlapped).
bool bigramCriteria30(const std::vector<uint8_t>& text,
                      const Statistics& statistics) {
  if (text.size() < 2U || statistics.text_size < 2U) {
    return true;
  }

  const std::size_t total = totalOverlappedBigrams(text.size());
  const std::size_t baseline_total =
      totalOverlappedBigrams(statistics.text_size);
  if (total == 0U || baseline_total == 0U) {
    return true;
  }

  std::unordered_map<uint16_t, uint32_t> sample_counts;
  sample_counts.reserve(std::min<std::size_t>(total, kFrequentBigrams));
  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    ++sample_counts[makeBigram(text[i], text[i + 1])];
  }

  const double sample_entropy =
      calculateEntropyFromCounts(sample_counts, static_cast<double>(total), 2U);
  const double baseline_entropy =
      calculateEntropyFromCounts(statistics.overlapped_bigrams_count,
                                 static_cast<double>(baseline_total), 2U);

  return std::abs(baseline_entropy - sample_entropy) > kEntropyThresholdBigrams;
}

// Criterion 5.1 (empty boxes, most frequent bigrams, overlapped).
bool bigramCriteria51(const std::vector<uint8_t>& text,
                      const Statistics& statistics) {
  if (text.size() < 2U || statistics.text_size < 2U) {
    return true;
  }

  std::vector<std::pair<uint16_t, uint32_t>> ordered;
  ordered.reserve(statistics.overlapped_bigrams_count.size());
  for (const auto& [bigram, count] : statistics.overlapped_bigrams_count) {
    ordered.push_back({bigram, count});
  }

  const std::size_t top_count =
      std::min<std::size_t>(kFrequentBigrams, ordered.size());
  if (top_count == 0U) {
    return true;
  }

  std::partial_sort(
      ordered.begin(), ordered.begin() + top_count, ordered.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

  std::unordered_set<uint16_t> frequent_bigrams;
  frequent_bigrams.reserve(top_count * 2U);
  for (std::size_t i = 0; i < top_count; ++i) {
    frequent_bigrams.insert(ordered[i].first);
  }

  std::unordered_set<uint16_t> seen;
  seen.reserve(top_count);
  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    const uint16_t bigram = makeBigram(text[i], text[i + 1]);
    if (frequent_bigrams.find(bigram) != frequent_bigrams.end()) {
      seen.insert(bigram);
      if (seen.size() == frequent_bigrams.size()) {
        break;
      }
    }
  }

  const std::size_t fempt = frequent_bigrams.size() - seen.size();
  return fempt >= kEmptyBigrams;
}

}  // namespace lab2

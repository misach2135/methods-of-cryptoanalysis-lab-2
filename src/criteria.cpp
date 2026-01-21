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

namespace {

uint16_t makeBigram(uint8_t first, uint8_t second) {
  return static_cast<uint16_t>(first * ALPHABET_SIZE + second);
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
                        size_t kp) {
  if (text.empty()) {
    return true;
  }

  std::unordered_set<uint8_t> appeared;
  appeared.reserve(std::min<std::size_t>(forbidden_symbols.size(), kp));

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) != forbidden_symbols.end()) {
      appeared.insert(symbol);
      if (appeared.size() >= kp) {
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

  if (statistics.text_size == 0U) {
    return true;
  }

  std::unordered_map<uint8_t, uint32_t> counts_local;
  counts_local.reserve(forbidden_symbols.size());

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) == forbidden_symbols.end()) {
      continue;
    }
    ++counts_local[symbol];
  }

  for (auto forbidden_symbol : forbidden_symbols) {
    auto count_local_iter = counts_local.find(forbidden_symbol);
    auto count_local = count_local_iter == counts_local.end()
                           ? 0.0
                           : (static_cast<double>(count_local_iter->second) /
                              static_cast<double>(text.size()));

    auto count_global_iter = statistics.counts.find(forbidden_symbol);
    auto count_global = count_global_iter == statistics.counts.end()
                            ? 0.0
                            : (static_cast<double>(count_global_iter->second) /
                               static_cast<double>(statistics.text_size));

    if (count_local > count_global) {
      return true;
    }
  }

  return false;
}

bool symbolicCriteria13(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        const Statistics& statistics) {
  if (text.empty()) {
    return true;
  }

  if (statistics.text_size == 0U) {
    return true;
  }

  std::unordered_map<uint8_t, uint32_t> counts_local;
  counts_local.reserve(forbidden_symbols.size());

  for (auto symbol : text) {
    if (forbidden_symbols.find(symbol) == forbidden_symbols.end()) {
      continue;
    }
    ++counts_local[symbol];
  }

  uint32_t count_f = 0U;
  uint32_t count_k = 0U;

  for (auto forbidden_symbol : forbidden_symbols) {
    auto count_local_iter = counts_local.find(forbidden_symbol);
    auto count_local = count_local_iter == counts_local.end()
                           ? 0.0
                           : (static_cast<double>(count_local_iter->second) /
                              static_cast<double>(text.size()));

    auto count_global_iter = statistics.counts.find(forbidden_symbol);
    auto count_global = count_global_iter == statistics.counts.end()
                            ? 0.0
                            : (static_cast<double>(count_global_iter->second) /
                               static_cast<double>(statistics.text_size));

    count_f += count_local;
    count_k += count_global;
  }

  return count_f > count_k;
}

bool symbolicCriteria30(const std::vector<uint8_t>& text,
                        const Statistics& statistics, const double threshold) {
  if (text.empty() || statistics.text_size == 0U) {
    return true;
  }

  std::unordered_map<uint8_t, uint32_t> counts_local;
  counts_local.reserve(ALPHABET_SIZE);

  calculateLetterFrequencies(text, counts_local);

  const double local_entropy = calculateEntropyL1(counts_local, text.size());

  return std::abs(statistics.entropy_1 - local_entropy) > threshold;
}

bool symbolicCriteria51(const std::vector<uint8_t>& text,
                        const Statistics& statistics, const size_t threshold,
                        const size_t j) {
  if (text.empty() || statistics.text_size == 0U) {
    return true;
  }

  if (statistics.counts.empty() || j == 0U) {
    return true;
  }

  std::vector<std::pair<int, int>> ordered(statistics.counts.begin(),
                                           statistics.counts.end());

  const std::size_t top_count = std::min<std::size_t>(j, ordered.size());
  if (top_count == 0U) {
    return true;
  }

  if (top_count < ordered.size()) {
    std::nth_element(
        ordered.begin(), ordered.begin() + top_count, ordered.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    ordered.resize(top_count);
  }

  std::unordered_set<uint8_t> sSet;
  sSet.reserve(ordered.size() * 2U);
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    sSet.insert(static_cast<uint8_t>(ordered[i].first));
  }

  std::unordered_map<uint8_t, uint32_t> boxes;
  boxes.reserve(ordered.size());
  for (const auto& [sym_i32, _cnt] : ordered) {
    boxes.emplace(static_cast<uint8_t>(sym_i32), 0U);
  }

  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto sym = text[i];
    if (sSet.find(sym) != sSet.end()) {
      boxes[sym]++;
    }
  }

  uint32_t ft = 0U;
  for (const auto& [key, val] : boxes) {
    if (val != 0U) {
      continue;
    }
    ft += 1U;
  }

  return ft >= threshold;
}

bool bigramCriteria10(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols) {
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

bool bigramCriteria11(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const uint32_t kp2) {
  if (text.size() < 2U) {
    return true;
  }

  std::unordered_set<uint16_t> appeared;
  appeared.reserve(std::min<std::size_t>(forbidden_symbols.size(), kp2));

  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    const uint16_t bigram = makeBigram(text[i], text[i + 1]);
    if (forbidden_symbols.find(bigram) != forbidden_symbols.end()) {
      appeared.insert(bigram);
      if (appeared.size() >= kp2) {
        return true;
      }
    }
  }

  return false;
}

bool bigramCriteria12(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics) {
  if (text.size() < 2U) {
    return true;
  }

  std::unordered_map<uint16_t, uint32_t> bigrams_count_local;

  calculateOverlappedBigramsCount(text, bigrams_count_local);

  for (auto forbidden_bigram : forbidden_symbols) {
    auto count_local_iter = bigrams_count_local.find(forbidden_bigram);
    auto count_local = count_local_iter == bigrams_count_local.end()
                           ? 0
                           : (static_cast<double>(count_local_iter->second) /
                              static_cast<double>(text.size() - 1));

    auto count_global_iter =
        statistics.overlapped_bigrams_count.find(forbidden_bigram);
    auto count_global =
        count_global_iter == statistics.overlapped_bigrams_count.end()
            ? 0
            : (static_cast<double>(count_global_iter->second) /
               static_cast<double>(statistics.text_size - 1));

    if (count_local > count_global) {
      return true;
    }
  }

  return false;
}

bool bigramCriteria13(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics) {
  if (text.size() < 2U) {
    return true;
  }

  std::unordered_map<uint16_t, uint32_t> bigrams_count_local;

  calculateOverlappedBigramsCount(text, bigrams_count_local);

  uint32_t count_f = 0;
  uint32_t count_k = 0;

  for (auto forbidden_bigram : forbidden_symbols) {
    auto count_local_iter = bigrams_count_local.find(forbidden_bigram);
    auto count_local = count_local_iter == bigrams_count_local.end()
                           ? 0
                           : count_local_iter->second;

    auto count_global_iter =
        statistics.overlapped_bigrams_count.find(forbidden_bigram);
    auto count_global =
        count_global_iter == statistics.overlapped_bigrams_count.end()
            ? 0
            : count_global_iter->second;

    count_f += count_local;
    count_k += count_global;
  }

  return count_f > count_k;
}

bool bigramCriteria30(const std::vector<uint8_t>& text,
                      const Statistics& statistics, const double threshold) {
  if (text.size() < 2U || statistics.text_size < 2U) {
    return true;
  }

  std::unordered_map<uint16_t, uint32_t> overlappedBigrams;

  calculateOverlappedBigramsCount(text, overlappedBigrams);

  double local_entropy = calculateEntropyL2(overlappedBigrams, text.size());

  return std::abs(statistics.entropy_2 - local_entropy) > threshold;
}

bool bigramCriteria51(const std::vector<uint8_t>& text,
                      const Statistics& statistics, const size_t threshold,
                      const size_t j) {
  if (text.size() < 2U || statistics.text_size < 2U) {
    return true;
  }

  std::vector<std::pair<int, int>> overlappedBigrams(
      statistics.overlapped_bigrams_count.begin(),
      statistics.overlapped_bigrams_count.end());

  const std::size_t top_count =
      std::min<std::size_t>(j, overlappedBigrams.size());
  if (top_count == 0U) {
    return true;
  }

  std::nth_element(
      overlappedBigrams.begin(), overlappedBigrams.begin() + top_count,
      overlappedBigrams.end(),
      [](const auto& a, const auto& b) { return a.second > b.second; });

  overlappedBigrams.resize(j);

  std::unordered_set<uint16_t> bSet;

  for (size_t i = 0; i < overlappedBigrams.size(); i++) {
    bSet.insert(overlappedBigrams[i].first);
  }

  std::unordered_map<uint16_t, uint32_t> boxes;

  for (const auto& [bigram_i32, _cnt] : overlappedBigrams) {
    boxes.emplace(static_cast<uint16_t>(bigram_i32), 0U);
  }

  for (size_t i = 0; i < text.size() - 1; i++) {
    uint16_t bigram = makeBigram(text[i], text[i + 1]);
    auto it = bSet.find(bigram);
    if (it != bSet.end()) {
      boxes[bigram]++;
    }
  }

  uint32_t ft = 0;

  for (auto const& [key, val] : boxes) {
    if (val != 0) continue;
    ft += 1;
  }

  return ft > threshold;
}

bool structuralCriteria(const size_t original_size,
                        const size_t compressed_size, const double threshold) {
  return getBitsPerSymbol(original_size, compressed_size) > threshold;
}

double getBitsPerSymbol(const size_t original_size,
                        const size_t compressed_size) {
  return static_cast<double>(compressed_size * 8) / original_size;
}

}  // namespace lab2

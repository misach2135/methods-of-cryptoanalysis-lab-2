#include "criteria.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

template <typename Gram>
using CountMap = std::unordered_map<Gram, uint32_t>;

template <typename Gram>
double sumCounts(const CountMap<Gram>& counts) {
  double total = 0.0;
  for (const auto& entry : counts) {
    total += static_cast<double>(entry.second);
  }
  return total;
}

template <typename Gram>
std::unordered_set<Gram> buildTopSet(const CountMap<Gram>& counts,
                                     std::size_t top_n) {
  std::vector<std::pair<Gram, uint32_t>> items;
  items.reserve(counts.size());
  for (const auto& entry : counts) {
    items.emplace_back(entry.first, entry.second);
  }
  std::sort(items.begin(), items.end(),
            [](const auto& left, const auto& right) {
              return left.second > right.second;
            });

  std::unordered_set<Gram> top_set;
  std::size_t limit = std::min(top_n, items.size());
  top_set.reserve(limit);
  for (std::size_t i = 0; i < limit; ++i) {
    top_set.insert(items[i].first);
  }
  return top_set;
}

uint16_t makeBigram(uint8_t first, uint8_t second) {
  return static_cast<uint16_t>(static_cast<uint16_t>(first) << 8) |
         static_cast<uint16_t>(second);
}

std::unordered_map<uint8_t, uint32_t> countSymbolWindow(
    const std::vector<uint8_t>& text, std::size_t window_size) {
  std::unordered_map<uint8_t, uint32_t> window_counts;
  window_counts.reserve(window_size);
  for (std::size_t i = 0; i < window_size; ++i) {
    ++window_counts[text[i]];
  }
  return window_counts;
}

std::unordered_map<uint16_t, uint32_t> countBigramWindow(
    const std::vector<uint8_t>& text, std::size_t window_size) {
  std::unordered_map<uint16_t, uint32_t> window_counts;
  if (window_size < 2) {
    return window_counts;
  }
  window_counts.reserve(window_size - 1);
  for (std::size_t i = 0; i + 1 < window_size; ++i) {
    ++window_counts[makeBigram(text[i], text[i + 1])];
  }
  return window_counts;
}

}  // namespace

bool lab2::symbolicCriteria10(const std::vector<uint8_t>& text,
                              const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size == 0) {
    return false;
  }

  auto top_set = buildTopSet(statistics.counts, kHp1);
  if (top_set.empty()) {
    return false;
  }

  for (std::size_t i = 0; i < window_size; ++i) {
    if (top_set.find(text[i]) != top_set.end()) {
      return true;
    }
  }
  return false;
}

bool lab2::symbolicCriteria11(const std::vector<uint8_t>& text,
                              const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size == 0) {
    return false;
  }

  auto top_set = buildTopSet(statistics.counts, kHp1);
  if (top_set.empty()) {
    return false;
  }

  std::unordered_set<uint8_t> accepted_set;
  accepted_set.reserve(top_set.size());
  for (std::size_t i = 0; i < window_size; ++i) {
    if (top_set.find(text[i]) != top_set.end()) {
      accepted_set.insert(text[i]);
      if (accepted_set.size() >= kKp1) {
        return true;
      }
    }
  }
  return false;
}

bool lab2::symbolicCriteria12(const std::vector<uint8_t>& text,
                              const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size == 0) {
    return false;
  }

  auto top_set = buildTopSet(statistics.counts, kHp1);
  if (top_set.empty()) {
    return false;
  }

  double baseline_total = sumCounts(statistics.counts);
  if (baseline_total == 0.0) {
    return false;
  }

  auto window_counts = countSymbolWindow(text, window_size);
  double window_total = static_cast<double>(window_size);
  if (window_total == 0.0) {
    return false;
  }

  for (const auto& gram : top_set) {
    auto baseline_it = statistics.counts.find(gram);
    if (baseline_it == statistics.counts.end()) {
      continue;
    }
    double k_x = static_cast<double>(baseline_it->second) / baseline_total;

    auto window_it = window_counts.find(gram);
    double f_x = 0.0;
    if (window_it != window_counts.end()) {
      f_x = static_cast<double>(window_it->second) / window_total;
    }

    if (f_x > k_x) {
      return true;
    }
  }
  return false;
}

bool lab2::symbolicCriteria13(const std::vector<uint8_t>& text,
                              const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size == 0) {
    return false;
  }

  auto top_set = buildTopSet(statistics.counts, kHp1);
  if (top_set.empty()) {
    return false;
  }

  double baseline_total = sumCounts(statistics.counts);
  if (baseline_total == 0.0) {
    return false;
  }

  auto window_counts = countSymbolWindow(text, window_size);
  double window_total = static_cast<double>(window_size);
  if (window_total == 0.0) {
    return false;
  }

  double k_sum = 0.0;
  double f_sum = 0.0;
  for (const auto& gram : top_set) {
    auto baseline_it = statistics.counts.find(gram);
    if (baseline_it != statistics.counts.end()) {
      k_sum += static_cast<double>(baseline_it->second) / baseline_total;
    }

    auto window_it = window_counts.find(gram);
    if (window_it != window_counts.end()) {
      f_sum += static_cast<double>(window_it->second) / window_total;
    }
  }

  return f_sum > k_sum;
}

bool lab2::symbolicCriteria30(const std::vector<uint8_t>& text,
                              const Statistics& statistics) {
  // TODO: Implement criterion 3.0.
  return false;
}

bool lab2::symbolicCriteria51(const std::vector<uint8_t>& text,
                              const Statistics& statistics) {
  // TODO: Implement criterion 5.1.
  return false;
}

bool lab2::bigramCriteria10(const std::vector<uint8_t>& text,
                            const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size < 2) {
    return false;
  }

  auto top_set = buildTopSet(statistics.overlapped_bigrams_count, kHp2);
  if (top_set.empty()) {
    return false;
  }

  for (std::size_t i = 0; i + 1 < window_size; ++i) {
    uint16_t key = makeBigram(text[i], text[i + 1]);
    if (top_set.find(key) != top_set.end()) {
      return true;
    }
  }
  return false;
}

bool lab2::bigramCriteria11(const std::vector<uint8_t>& text,
                            const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size < 2) {
    return false;
  }

  auto top_set = buildTopSet(statistics.overlapped_bigrams_count, kHp2);
  if (top_set.empty()) {
    return false;
  }

  std::unordered_set<uint16_t> accepted_set;
  accepted_set.reserve(top_set.size());
  for (std::size_t i = 0; i + 1 < window_size; ++i) {
    uint16_t key = makeBigram(text[i], text[i + 1]);
    if (top_set.find(key) != top_set.end()) {
      accepted_set.insert(key);
      if (accepted_set.size() >= kKp2) {
        return true;
      }
    }
  }
  return false;
}

bool lab2::bigramCriteria12(const std::vector<uint8_t>& text,
                            const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size < 2) {
    return false;
  }

  auto top_set = buildTopSet(statistics.overlapped_bigrams_count, kHp2);
  if (top_set.empty()) {
    return false;
  }

  double baseline_total = sumCounts(statistics.overlapped_bigrams_count);
  if (baseline_total == 0.0) {
    return false;
  }

  auto window_counts = countBigramWindow(text, window_size);
  double window_total = static_cast<double>(window_size - 1);
  if (window_total == 0.0) {
    return false;
  }

  for (const auto& gram : top_set) {
    auto baseline_it = statistics.overlapped_bigrams_count.find(gram);
    if (baseline_it == statistics.overlapped_bigrams_count.end()) {
      continue;
    }
    double k_x = static_cast<double>(baseline_it->second) / baseline_total;

    auto window_it = window_counts.find(gram);
    double f_x = 0.0;
    if (window_it != window_counts.end()) {
      f_x = static_cast<double>(window_it->second) / window_total;
    }

    if (f_x > k_x) {
      return true;
    }
  }
  return false;
}

bool lab2::bigramCriteria13(const std::vector<uint8_t>& text,
                            const Statistics& statistics) {
  std::size_t window_size = std::min(text.size(), kWindowL);
  if (window_size < 2) {
    return false;
  }

  auto top_set = buildTopSet(statistics.overlapped_bigrams_count, kHp2);
  if (top_set.empty()) {
    return false;
  }

  double baseline_total = sumCounts(statistics.overlapped_bigrams_count);
  if (baseline_total == 0.0) {
    return false;
  }

  auto window_counts = countBigramWindow(text, window_size);
  double window_total = static_cast<double>(window_size - 1);
  if (window_total == 0.0) {
    return false;
  }

  double k_sum = 0.0;
  double f_sum = 0.0;
  for (const auto& gram : top_set) {
    auto baseline_it = statistics.overlapped_bigrams_count.find(gram);
    if (baseline_it != statistics.overlapped_bigrams_count.end()) {
      k_sum += static_cast<double>(baseline_it->second) / baseline_total;
    }

    auto window_it = window_counts.find(gram);
    if (window_it != window_counts.end()) {
      f_sum += static_cast<double>(window_it->second) / window_total;
    }
  }

  return f_sum > k_sum;
}

bool lab2::bigramCriteria30(const std::vector<uint8_t>& text,
                            const Statistics& statistics) {
  // TODO: Implement criterion 3.0.
  return false;
}

bool lab2::bigramCriteria51(const std::vector<uint8_t>& text,
                            const Statistics& statistics) {
  // TODO: Implement criterion 5.1.
  return false;
}

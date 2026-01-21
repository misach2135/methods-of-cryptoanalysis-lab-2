#ifndef LAB2_STATISTICS_H
#define LAB2_STATISTICS_H

#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lab2 {

struct Statistics {
  uint32_t text_size;
  double entropy_1;
  double entropy_2;
  double index_of_coincidence_1;
  double index_of_coincidence_2;

  std::unordered_map<uint8_t, uint32_t> counts;
  std::unordered_map<uint16_t, uint32_t> overlapped_bigrams_count;
  std::unordered_map<uint16_t, uint32_t> non_overlapped_bigrams_count;
};

Statistics calculateStatistics(const std::vector<uint8_t>& bytes);

void calculateLetterFrequencies(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint8_t, uint32_t>& letter_counts);

void calculateOverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count);

void calculateNonoverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& non_overlapped_bigrams_count);

double calculateEntropyL1(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size);

double calculateEntropyL2(
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const uint32_t text_size);

double calculateIndexOfCoincidenceL1(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size);

double calculateIndexOfCoincidenceL2(
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const uint32_t text_size);

void calculateForbiddenBigrams(
    std::unordered_set<uint16_t>& forbidden_bigrams,
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const double threshold);

void calculateForbiddenSymbols(
    std::unordered_set<uint8_t>& forbidden_symbols,
    const std::unordered_map<uint8_t, uint32_t>& symbol_counts,
    const double threshold);

}  // namespace lab2

template <>
struct fmt::formatter<lab2::Statistics> : fmt::formatter<std::string> {
  auto format(lab2::Statistics statistics, fmt::format_context& ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(),
                          "Text size = {},\n"
                          "Letters = {}/33\n"
                          "Overlapped bigrams = {},\n"
                          "Non-overlapped bigrams = {},\n"
                          "Entropy(l=1) = {},\n"
                          "Entropy(l=2) = {},\n"
                          "Index of coincidence(l=1) = {}.\n"
                          "Index of coincidence(l=2) = {}.\n",
                          statistics.text_size, statistics.counts.size(),
                          statistics.overlapped_bigrams_count.size(),
                          statistics.non_overlapped_bigrams_count.size(),
                          statistics.entropy_1, statistics.entropy_2,
                          statistics.index_of_coincidence_1,
                          statistics.index_of_coincidence_2);
  }
};

template <typename T>
struct fmt::formatter<std::unordered_set<T>> {
  constexpr auto parse(fmt::format_parse_context& ctx)
      -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(const std::unordered_set<T>& set, fmt::format_context& ctx) const
      -> decltype(ctx.out()) {
    auto out = ctx.out();
    out = fmt::format_to(out, "{{");

    bool first = true;
    for (const auto& v : set) {
      if (!first) {
        out = fmt::format_to(out, ", ");
      }
      first = false;

      out = fmt::format_to(out, "{}", v);
    }

    out = fmt::format_to(out, "}}");
    return out;
  }
};

#endif

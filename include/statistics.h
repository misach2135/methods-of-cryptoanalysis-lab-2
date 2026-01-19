#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lab2 {

struct Statistics {
  uint32_t text_size;
  double entropy;
  double index_of_coincidence;

  std::unordered_map<uint8_t, uint32_t> counts;
  std::unordered_map<uint16_t, uint32_t> overlapped_bigrams_count;
  std::unordered_map<uint16_t, uint32_t> non_overlapped_bigrams_count;
  std::unordered_set<uint16_t> forbidden_bigrams;
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

double calculateEntropy(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size);

double calculateIndexOfCoincidence(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size);

void calculateForbiddenBigrams(
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const uint32_t text_size, const uint32_t threshold);

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
                          "Entropy = {},\n"
                          "Index of coincidence = {}.\n",
                          statistics.text_size, statistics.counts.size(),
                          statistics.overlapped_bigrams_count.size(),
                          statistics.non_overlapped_bigrams_count.size(),
                          statistics.entropy, statistics.index_of_coincidence);
  }
};
#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace lab2 {

struct Statistics {
  std::unordered_map<uint8_t, uint32_t> counts;
  std::unordered_map<uint16_t, uint32_t> overlapped_bigrams_count;
  std::unordered_map<uint16_t, uint32_t> non_overlapped_bigrams_count;

  double entropy;
  double index_of_coincidence;
};

Statistics calculateStatistics(const std::vector<uint8_t>& bytes);

void calculateLetterFrequencies(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint8_t, uint32_t>& letterCounts);

uint32_t calculateOverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& overlappedBigramsCount);

uint32_t calculateNonoverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& nonOverlappedBigramsCount);

double calculateEntropy(
    const std::unordered_map<uint8_t, uint32_t>& letterCounts);

double calculateIndexOfCoincidence();

}  // namespace lab2

template <>
struct fmt::formatter<lab2::Statistics> : fmt::formatter<std::string> {
  auto format(lab2::Statistics statistics, fmt::format_context& ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(),
                          "Letters = {}/33\n"
                          "Overlapped bigrams = {},\n"
                          "Non-overlapped bigrams = {},\n"
                          "Entropy = {},\n"
                          "Index of coincidence = {}.\n",
                          statistics.counts.size(),
                          statistics.overlapped_bigrams_count.size(),
                          statistics.non_overlapped_bigrams_count.size(),
                          statistics.entropy, statistics.index_of_coincidence);
  }
};
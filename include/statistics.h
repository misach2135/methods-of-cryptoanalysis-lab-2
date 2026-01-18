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
  double compliance_index;
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
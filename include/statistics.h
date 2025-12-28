#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace lab2 {

struct Statistics {
  std::unordered_map<uint8_t, uint64_t> counts;
  std::unordered_map<uint16_t, uint64_t> overlapped_bigrams_count;
  std::unordered_map<uint16_t, uint64_t> non_overlapped_bigrams_count;

  double entropy;
  double compliance_index;
};

Statistics calculateStatistics(std::vector<uint8_t> bytes);

}  // namespace lab2
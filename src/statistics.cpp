#include "statistics.h"

lab2::Statistics lab2::calculateStatistics(const std::vector<uint8_t>& bytes) {
  return Statistics();
}

void lab2::calculateLetterFrequencies(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint8_t, uint32_t>& letterCounts) {}

uint32_t lab2::calculateOverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& overlappedBigramsCount) {
  return 0;
}

uint32_t lab2::calculateNonoverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& nonOverlappedBigramsCount) {
  return 0;
}

double lab2::calculateEntropy(
    const std::unordered_map<uint8_t, uint32_t>& letterCounts) {
  return 0.0;
}

double lab2::calculateIndexOfCoincidence() { return 0.0; }

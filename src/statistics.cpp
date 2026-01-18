#include "statistics.h"

#include <spdlog/spdlog.h>

#include <cmath>
#include <thread>

lab2::Statistics lab2::calculateStatistics(const std::vector<uint8_t>& bytes) {
  std::unordered_map<uint8_t, uint32_t> counts;
  std::unordered_map<uint16_t, uint32_t> overlapped_bigrams_count;
  std::unordered_map<uint16_t, uint32_t> non_overlapped_bigrams_count;

  auto t1 =
      std::thread([&]() { lab2::calculateLetterFrequencies(bytes, counts); });
  auto t2 = std::thread([&]() {
    lab2::calculateOverlappedBigramsCount(bytes, overlapped_bigrams_count);
  });
  auto t3 = std::thread([&]() {
    lab2::calculateNonoverlappedBigramsCount(bytes,
                                             non_overlapped_bigrams_count);
  });

  t1.join();
  t2.join();
  t3.join();

  uint32_t text_size = static_cast<uint32_t>(bytes.size());
  double entropy = lab2::calculateEntropy(counts, bytes.size());
  double index_of_coincidence =
      lab2::calculateIndexOfCoincidence(counts, bytes.size());

  return Statistics{text_size,
                    entropy,
                    index_of_coincidence,
                    counts,
                    overlapped_bigrams_count,
                    non_overlapped_bigrams_count};
}

void lab2::calculateLetterFrequencies(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint8_t, uint32_t>& letterCounts) {
  for (auto it : bytes) {
    ++letterCounts[it];
  }
}

void lab2::calculateOverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& overlappedBigramsCount) {
  for (size_t i = 0; i < bytes.size() - 1; ++i) {
    uint16_t bigram = static_cast<uint16_t>(bytes[i] << 8) |
                      static_cast<uint16_t>(bytes[i + 1]);
    ++overlappedBigramsCount[bigram];
  }
}

void lab2::calculateNonoverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& nonOverlappedBigramsCount) {
  for (size_t i = 0; i <= bytes.size() / 2; i += 2) {
    uint16_t bigram = static_cast<uint16_t>(bytes[i] << 8) |
                      static_cast<uint16_t>(bytes[i + 1]);
    ++nonOverlappedBigramsCount[bigram];
  }
}

double lab2::calculateEntropy(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size) {
  double entropy = 0.0;
  for (const auto& [letter, count] : letter_counts) {
    double prob = static_cast<double>(count) / static_cast<double>(text_size);
    entropy += -prob * log2(prob);
  }

  return entropy;
}

double lab2::calculateIndexOfCoincidence(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size) {
  double index_of_coincidence = 0.0;

  for (const auto& [letter, count] : letter_counts) {
    index_of_coincidence += static_cast<double>(count * (count - 1)) /
                            static_cast<double>(text_size);
  }

  return index_of_coincidence;
}

#include "statistics.h"

#include <spdlog/spdlog.h>

#include <cmath>
#include <thread>

#include "alphabet.h"

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
  double entropy_1 = lab2::calculateEntropyL1(counts, bytes.size());
  double entropy_2 =
      lab2::calculateEntropyL2(overlapped_bigrams_count, bytes.size());
  double index_of_coincidence_1 =
      lab2::calculateIndexOfCoincidenceL1(counts, bytes.size());

  double index_of_coincidence_2 = lab2::calculateIndexOfCoincidenceL2(
      overlapped_bigrams_count, bytes.size());

  return Statistics{
      text_size,
      entropy_1,
      entropy_2,
      index_of_coincidence_1,
      index_of_coincidence_2,
      counts,
      overlapped_bigrams_count,
      non_overlapped_bigrams_count,
  };
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
  for (size_t i = 0; i + 1 < bytes.size(); ++i) {
    uint16_t bigram = bytes[i] * ALPHABET_SIZE + bytes[i + 1];
    ++overlappedBigramsCount[bigram];
  }
}

void lab2::calculateNonoverlappedBigramsCount(
    const std::vector<uint8_t>& bytes,
    std::unordered_map<uint16_t, uint32_t>& nonOverlappedBigramsCount) {
  for (size_t i = 0; i + 1 < bytes.size(); i += 2) {
    uint16_t bigram = bytes[i] * ALPHABET_SIZE + bytes[i + 1];
    ++nonOverlappedBigramsCount[bigram];
  }
}

double lab2::calculateEntropyL1(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size) {
  if (text_size == 0U) {
    return 0.0;
  }
  double entropy = 0.0;
  for (const auto& [_, count] : letter_counts) {
    double prob = static_cast<double>(count) / static_cast<double>(text_size);
    entropy += -prob * log2(prob);
  }

  return entropy;
}

double lab2::calculateEntropyL2(
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const uint32_t text_size) {
  if (text_size == 0U) {
    return 0.0;
  }
  double entropy = 0.0;
  for (const auto& [_, count] : overlapped_bigrams_count) {
    double prob =
        static_cast<double>(count) / static_cast<double>(text_size - 1);
    entropy += -prob * log2(prob);
  }

  return entropy / 2;
}

double lab2::calculateIndexOfCoincidenceL1(
    const std::unordered_map<uint8_t, uint32_t>& letter_counts,
    const uint32_t text_size) {
  if (text_size == 0U) {
    return 0.0;
  }
  double index_of_coincidence = 0.0;

  for (const auto& [letter, count] : letter_counts) {
    index_of_coincidence += static_cast<double>(count * (count - 1));
  }

  index_of_coincidence /=
      (static_cast<double>(text_size) * static_cast<double>(text_size - 1));

  return index_of_coincidence;
}

double lab2::calculateIndexOfCoincidenceL2(
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const uint32_t text_size) {
  if (text_size == 0U) {
    return 0.0;
  }
  double index_of_coincidence = 0.0;

  for (const auto& [_, count] : overlapped_bigrams_count) {
    index_of_coincidence += static_cast<double>(count * (count - 1));
  }

  index_of_coincidence /=
      (static_cast<double>(text_size) * static_cast<double>(text_size - 1));

  return index_of_coincidence;
}

void lab2::calculateForbiddenBigrams(
    std::unordered_set<uint16_t>& forbidden_bigrams,
    const std::unordered_map<uint16_t, uint32_t>& overlapped_bigrams_count,
    const double threshold, const size_t text_size) {
  for (uint16_t bigram = 0; bigram < ALPHABET_SIZE * ALPHABET_SIZE; bigram++) {
    auto it = overlapped_bigrams_count.find(bigram);
    double count = (it == overlapped_bigrams_count.end())
                       ? 0
                       : static_cast<double>(it->second);

    count /= static_cast<double>(text_size);

    if (count <= threshold) {
      forbidden_bigrams.insert(bigram);
    }
  }
}

void lab2::calculateForbiddenSymbols(
    std::unordered_set<uint8_t>& forbidden_symbols,
    const std::unordered_map<uint8_t, uint32_t>& symbol_counts,
    const double threshold, const size_t text_size) {
  for (uint8_t c = 0; c < ALPHABET_SIZE; c++) {
    auto it = symbol_counts.find(c);
    double count =
        (it == symbol_counts.end()) ? 0 : static_cast<double>(it->second);

    count /= text_size - 1;
    if (count <= threshold) {
      forbidden_symbols.insert(c);
    }
  }
}

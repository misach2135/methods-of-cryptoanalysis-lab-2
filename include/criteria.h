#ifndef LAB2_CRITERIA
#define LAB2_CRITERIA

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "statistics.h"

namespace lab2 {

bool symbolicCriteria10(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols);

bool symbolicCriteria11(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        size_t kp);

bool symbolicCriteria12(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        const Statistics& statistics);

bool symbolicCriteria13(const std::vector<uint8_t>& text,
                        const std::unordered_set<uint8_t>& forbidden_symbols,
                        const Statistics& statistics);

bool symbolicCriteria30(const std::vector<uint8_t>& text,
                        const Statistics& statistic,
                        const double entropyThresholdSymbols);

bool symbolicCriteria51(const std::vector<uint8_t>& text,
                        const Statistics& statistics, const size_t threshold,
                        const size_t j);

bool bigramCriteria10(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols);
bool bigramCriteria11(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const uint32_t kp2);
bool bigramCriteria12(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics);
bool bigramCriteria13(const std::vector<uint8_t>& text,
                      const std::unordered_set<uint16_t>& forbidden_symbols,
                      const Statistics& statistics);
bool bigramCriteria30(const std::vector<uint8_t>& text,
                      const Statistics& statistics, const double threshold);
bool bigramCriteria51(const std::vector<uint8_t>& text,
                      const Statistics& statistics, const double threshold,
                      const size_t j);

bool structuralCriteria(const std::vector<uint8_t>& text);

}  // namespace lab2

#endif

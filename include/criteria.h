#ifndef LAB2_CRITERIA
#define LAB2_CRITERIA

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

#include <cstdint>
#include <iostream>
#include <vector>

#include "statistics.h"

namespace lab2 {

bool symbolicCriteria10(const std::vector<uint8_t>& text,
                        const Statistics& statistics);
bool symbolicCriteria11(const std::vector<uint8_t>& text,
                        const Statistics& statistics);
bool symbolicCriteria12(const std::vector<uint8_t>& text,
                        const Statistics& statistics);
bool symbolicCriteria13(const std::vector<uint8_t>& text,
                        const Statistics& statistics);
bool symbolicCriteria30(const std::vector<uint8_t>& text,
                        const Statistics& statistics);
bool symbolicCriteria51(const std::vector<uint8_t>& text,
                        const Statistics& statistics);

bool bigramCriteria10(const std::vector<uint8_t>& text,
                      const Statistics& statistics);
bool bigramCriteria11(const std::vector<uint8_t>& text,
                      const Statistics& statistics);
bool bigramCriteria12(const std::vector<uint8_t>& text,
                      const Statistics& statistics);
bool bigramCriteria13(const std::vector<uint8_t>& text,
                      const Statistics& statistics);
bool bigramCriteria30(const std::vector<uint8_t>& text,
                      const Statistics& statistics);
bool bigramCriteria51(const std::vector<uint8_t>& text,
                      const Statistics& statistics);

}  // namespace lab2

#endif
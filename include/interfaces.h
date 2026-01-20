#ifndef LAB2_CRITERIA_INTERFACES
#define LAB2_CRITERIA_INTERFACES

#include <cstdint>
#include <vector>

#include "statistics.h"

struct CriteriaResult {
  uint32_t h0_count;  // natural text hypothesis
  uint32_t h1_count;  // random text hypothesis
};

CriteriaResult applySymbolCriteria10(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint8_t>& forbidden_symbols);

CriteriaResult applySymbolCriteria11(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint8_t>& forbidden_symbols, size_t threshold);

CriteriaResult applySymbolCriteria12(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint8_t>& forbidden_symbols,
    const lab2::Statistics& statistics);

CriteriaResult applySymbolCriteria13(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint8_t>& forbidden_symbols,
    const lab2::Statistics& statistics);

CriteriaResult applySymbolCriteria30(const std::vector<uint8_t> plain_texts,
                                     const lab2::Statistics& statistics);

CriteriaResult applySymbolCriteria51(const std::vector<uint8_t> plain_texts,
                                     const lab2::Statistics& statistics,
                                     uint32_t j);

CriteriaResult applyBigramCriteria10(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams);

CriteriaResult applyBigramCriteria11(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams, size_t threshold);

CriteriaResult applyBigramsCriteria12(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams,
    const lab2::Statistics& statistics);

CriteriaResult applyBigramCriteria13(
    const std::vector<uint8_t> plain_texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams,
    const lab2::Statistics& statistics);

CriteriaResult applyBigramCriteria30(const std::vector<uint8_t> plain_texts,
                                     const lab2::Statistics& statistics);

CriteriaResult applyBigramCriteria51(const std::vector<uint8_t> plain_texts,
                                     const lab2::Statistics& statistics,
                                     uint32_t j);

#endif
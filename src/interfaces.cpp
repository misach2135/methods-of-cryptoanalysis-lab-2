#include "interfaces.h"

#include "criteria.h"

namespace {

template <typename CriteriaFunc>
CriteriaResult applyCriteriaToTexts(
    const std::vector<std::vector<uint8_t>>& texts, CriteriaFunc&& criteria) {
  CriteriaResult result{0U, 0U};
  for (const auto& text : texts) {
    if (criteria(text)) {
      ++result.h1_count;
    } else {
      ++result.h0_count;
    }
  }
  return result;
}

}  // namespace

CriteriaResult applySymbolCriteria10(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint8_t>& forbidden_symbols) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::symbolicCriteria10(text, forbidden_symbols);
  });
}

CriteriaResult applySymbolCriteria11(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint8_t>& forbidden_symbols, size_t threshold) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::symbolicCriteria11(text, forbidden_symbols, threshold);
  });
}

CriteriaResult applySymbolCriteria12(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint8_t>& forbidden_symbols,
    const lab2::Statistics& statistics) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::symbolicCriteria12(text, forbidden_symbols, statistics);
  });
}

CriteriaResult applySymbolCriteria13(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint8_t>& forbidden_symbols,
    const lab2::Statistics& statistics) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::symbolicCriteria13(text, forbidden_symbols, statistics);
  });
}

CriteriaResult applySymbolCriteria30(
    const std::vector<std::vector<uint8_t>> texts,
    const lab2::Statistics& statistics) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::symbolicCriteria30(text, statistics, 0.0);
  });
}

CriteriaResult applySymbolCriteria51(
    const std::vector<std::vector<uint8_t>> texts,
    const lab2::Statistics& statistics, uint32_t j) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::symbolicCriteria51(text, statistics, static_cast<size_t>(j),
                                    4);
  });
}

CriteriaResult applyBigramCriteria10(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::bigramCriteria10(text, forbidden_bigrams);
  });
}

CriteriaResult applyBigramCriteria11(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams, size_t threshold) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::bigramCriteria11(text, forbidden_bigrams,
                                  static_cast<uint32_t>(threshold));
  });
}

CriteriaResult applyBigramsCriteria12(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams,
    const lab2::Statistics& statistics) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::bigramCriteria12(text, forbidden_bigrams, statistics);
  });
}

CriteriaResult applyBigramCriteria13(
    const std::vector<std::vector<uint8_t>> texts,
    const std::unordered_set<uint16_t>& forbidden_bigrams,
    const lab2::Statistics& statistics) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::bigramCriteria13(text, forbidden_bigrams, statistics);
  });
}

CriteriaResult applyBigramCriteria30(
    const std::vector<std::vector<uint8_t>> texts,
    const lab2::Statistics& statistics) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::bigramCriteria30(text, statistics, 0U);
  });
}

CriteriaResult applyBigramCriteria51(
    const std::vector<std::vector<uint8_t>> texts,
    const lab2::Statistics& statistics, uint32_t j) {
  return applyCriteriaToTexts(texts, [&](const std::vector<uint8_t>& text) {
    return lab2::bigramCriteria51(text, statistics, static_cast<size_t>(j), 4);
  });
}

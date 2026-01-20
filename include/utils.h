#ifndef LAB2_UTILS
#define LAB2_UTILS

#include <zlib.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<uint8_t> compressText(const std::vector<uint8_t>& text) {
  uLong src_len = text.size();
  uLong dst_len = compressBound(src_len);

  std::vector<uint8_t> compressed(dst_len);

  int res = compress(compressed.data(), &dst_len,
                     reinterpret_cast<const Bytef*>(text.data()), src_len);

  if (res != Z_OK) {
    throw std::runtime_error("Compression failed");
  }

  compressed.resize(dst_len);  // important
  return compressed;
}

std::vector<uint8_t> decompressText(const std::vector<uint8_t>& compressed,
                                    size_t original_size) {
  std::vector<uint8_t> decompressed(original_size);

  uLong dst_len = original_size;

  int res = uncompress(decompressed.data(), &dst_len, compressed.data(),
                       compressed.size());

  if (res != Z_OK) {
    throw std::runtime_error("Decompression failed");
  }

  return decompressed;
}

#endif
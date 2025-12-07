#pragma once
#include "compressor.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

class RLECompressor : public Compressor {
public:
  static constexpr uint8_t ID = 1;
  static constexpr const char *NAME = "rle";

  void compress(const std::vector<std::filesystem::path> &files) override;
  void decompress(const std::vector<std::filesystem::path> &files) override;

  struct DecodedFile {
    std::string name;
    std::vector<uint8_t> data;
  };

  std::vector<uint8_t> rle_encode(const std::vector<uint8_t> &bytes);
  std::vector<DecodedFile> rle_decode(const std::vector<uint8_t> &bytes);
};

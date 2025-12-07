#include "rle.hpp"

#include "compressor_factory.hpp"
#include "utils.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
bool registered = []() {
  CompressorFactory::register_compressor(
      RLECompressor::ID, RLECompressor::NAME,
      []() { return std::make_unique<RLECompressor>(); });
  return true;
}();
} // namespace

void RLECompressor::compress(const std::vector<std::filesystem::path> &files) {
  if (files.empty()) {
    throw std::runtime_error("No files provided");
  }

  std::vector<uint8_t> output;

  // Global header
  uint8_t compression_type = static_cast<uint8_t>(RLECompressor::ID);
  output.push_back(compression_type);

  for (auto &file : files) {
    auto bytes = read_bytes(file);

    uint32_t uncompressed_block_length = bytes.size();

    auto compressed_block = rle_encode(bytes);

    uint32_t compressed_block_length = compressed_block.size();

    // File header
    std::string filename_string = file.filename().string();
    if (filename_string.size() > 255) {
      throw std::runtime_error("Filename too long (max 255 characters): " +
                               filename_string);
    }
    output.push_back(static_cast<uint8_t>(filename_string.size()));
    for (auto &character : filename_string) {
      output.push_back(static_cast<uint8_t>(character));
    }

    // Uncompressed bytes size
    output.push_back(uncompressed_block_length & 0xFF);
    output.push_back((uncompressed_block_length >> 8) & 0xFF);
    output.push_back((uncompressed_block_length >> 16) & 0xFF);
    output.push_back((uncompressed_block_length >> 24) & 0xFF);

    // Compressed bytes size
    output.push_back(compressed_block_length & 0xFF);
    output.push_back((compressed_block_length >> 8) & 0xFF);
    output.push_back((compressed_block_length >> 16) & 0xFF);
    output.push_back((compressed_block_length >> 24) & 0xFF);

    // Compressed bytes
    output.insert(output.end(), compressed_block.begin(),
                  compressed_block.end());
  }

  auto compressed_output = files[0];
  compressed_output.replace_extension(".leo");
  write_bytes(compressed_output, output);
}

void RLECompressor::decompress(
    const std::vector<std::filesystem::path> &files) {
  for (auto &file : files) {
    auto bytes = read_bytes(file);

    if (bytes.empty()) {
      std::cerr << "File " << file << " is empty and couldn't be read."
                << std::endl;
      break;
    }

    auto decompressed_file = rle_decode(bytes);

    std::filesystem::path dir = file.parent_path() / file.stem();
    std::filesystem::create_directories(dir);

    for (auto &df : decompressed_file) {
      std::filesystem::path path = dir / df.name;
      write_bytes(path, df.data);
    }
  }
}

std::vector<uint8_t>
RLECompressor::rle_encode(const std::vector<uint8_t> &bytes) {
  std::vector<uint8_t> compressed_block;
  size_t i = 0;
  // 0 to 127 == literal
  // 129 to 255 == run_length
  while (i < bytes.size()) {
    size_t run_length = 1;
    while (i + run_length < bytes.size() && bytes[i + run_length] == bytes[i] &&
           run_length < 128) {
      run_length++;
    }

    if (run_length > 2) {
      // Run
      compressed_block.push_back(127 + run_length);
      compressed_block.push_back(bytes[i]);
      i += run_length;
    } else {
      // Literal
      std::vector<uint8_t> literal;
      literal.push_back(bytes[i]);
      i++;

      while (i < bytes.size()) {
        if (i + 1 < bytes.size() && bytes[i] == bytes[i + 1]) {
          break;
        }
        literal.push_back(bytes[i]);
        i++;
        if (literal.size() == 128) {
          break;
        }
      }
      compressed_block.push_back(literal.size() - 1);
      compressed_block.insert(compressed_block.end(), literal.begin(),
                              literal.end());
    }
  }
  return compressed_block;
}

std::vector<RLECompressor::DecodedFile>
RLECompressor::rle_decode(const std::vector<uint8_t> &bytes) {
  std::vector<RLECompressor::DecodedFile> output;
  size_t byte_index = 1;

  while (byte_index < bytes.size()) {
    // Name
    uint8_t filename_length = bytes[byte_index++];
    if (byte_index + filename_length > bytes.size()) {
      throw std::runtime_error(
          "Corrupted file: filename extends beyond file end");
    }
    std::string filename(reinterpret_cast<const char *>(&bytes[byte_index]),
                         filename_length);
    byte_index += filename_length;

    // Length
    if (byte_index + 4 > bytes.size()) {
      throw std::runtime_error(
          "Corrupted file: unexpected end while reading block length");
    }
    uint32_t uncompressed_block_length =
        (bytes[byte_index]) | (bytes[byte_index + 1] << 8) |
        (bytes[byte_index + 2] << 16) | (bytes[byte_index + 3] << 24);

    byte_index += 4;

    uint32_t compressed_block_length =
        (bytes[byte_index]) | (bytes[byte_index + 1] << 8) |
        (bytes[byte_index + 2] << 16) | (bytes[byte_index + 3] << 24);

    byte_index += 4;

    // Copy
    if (byte_index + compressed_block_length > bytes.size()) {
      throw std::runtime_error(
          "Corrupted file: compressed block extends beyond file end");
    }

    std::vector<uint8_t> compressed_block(bytes.begin() + byte_index,
                                          bytes.begin() + byte_index +
                                              compressed_block_length);
    byte_index += compressed_block_length;

    // Decode
    RLECompressor::DecodedFile file;
    file.name = filename;
    file.data.reserve(uncompressed_block_length);

    size_t i = 0;
    while (i < compressed_block.size()) {
      uint8_t header = compressed_block[i++];
      if (header < 128) {
        // Literal
        uint8_t literal_length = header + 1;
        if (i + literal_length > compressed_block.size()) {
          throw std::runtime_error(
              "Corrupted file: literal run extends beyond block end");
        }

        file.data.insert(file.data.end(), compressed_block.begin() + i,
                         compressed_block.begin() + i + literal_length);
        i += literal_length;
      } else {
        // Run
        if (i >= compressed_block.size()) {
          throw std::runtime_error(
              "Corrupted file: unexpected end while reading run value");
        }

        uint8_t count = header - 127;
        uint8_t value = compressed_block[i++];
        file.data.insert(file.data.end(), count, value);
      }
    }
    output.push_back(std::move(file));
  }
  return output;
}

#include "aria.h"
#include "util.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <random>

template <class T, class U>
concept Derived = std::is_base_of<U, T>::value;

std::string bytesToString(const std::array<uint8_t, BLOCK_SIZE> bytes) {
  std::string str;
  str.assign(reinterpret_cast<std::string::value_type *>(
                 const_cast<std::uint8_t *>(bytes.data())),
             bytes.size() / sizeof(std::string::value_type));
  return str;
}

void getRandomBytes(std::array<uint8_t, BLOCK_SIZE_BYTES> &block) {
  static thread_local std::mt19937 rng{std::random_device{}()};

  int offset = 0;
  uint32_t bits = 0;
  for (uint8_t &d : block) {
    if (offset == 0)
      bits = rng();
    d = static_cast<uint8_t>(bits & 0xFF);
    bits >>= 8;
    if (++offset >= 4)
      offset = 0;
  }
}

template <size_t S> void getRandomBlock(std::array<chunk, S> &block) {
  static thread_local std::mt19937 rng{std::random_device{}()};

  for (int i = 0; i < S; i++) {
    block[i] = (chunk)rng();
  }
}

template <Derived<AriaKey> K, size_t S> K getRandomKey() {
  std::array<chunk, S> data = {};

  getRandomBlock<S>(data);

  return K(data);
}

template <Derived<AriaKey> K, size_t S>
void writeKeysToFile(std::filesystem::path path, unsigned int amount) {
  std::ofstream out;
  out.open(path, std::ios::binary);
  if (!out)
    throw std::runtime_error("Could not open key file");

  for (unsigned int i = 0; i < amount; i++) {
    const AriaKey &key = getRandomKey<K, S>();

    for (uint32_t word : key.getValue()) {
      const uint8_t bytes[4] = {
          static_cast<uint8_t>(word >> 24), static_cast<uint8_t>(word >> 16),
          static_cast<uint8_t>(word >> 8), static_cast<uint8_t>(word)};

      out.write(reinterpret_cast<const char *>(bytes), sizeof(bytes));
    }
  }
  out.close();
}

void writeBlocksToFile(std::filesystem::path path, unsigned int amount) {
  std::ofstream out;
  out.open(path, std::ios::binary);
  if (!out)
    throw std::runtime_error("Could not open blocks file");

  for (unsigned int i = 0; i < amount; i++) {
    std::array<chunk, BLOCK_SIZE> block{};
    getRandomBlock(block);

    for (uint32_t word : block) {
      const uint8_t bytes[4] = {
          static_cast<uint8_t>(word >> 24), static_cast<uint8_t>(word >> 16),
          static_cast<uint8_t>(word >> 8), static_cast<uint8_t>(word)};

      out.write(reinterpret_cast<const char *>(bytes), sizeof(bytes));
    }
  }
  out.close();
}

template <Derived<AriaKey> K, size_t S>
std::vector<K> readKeysFromFile(std::filesystem::path path) {
  std::ifstream in;
  in.open(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("Could not open key file");

  std::vector<K> keys = {};

  while (in.peek() != EOF) {
    std::array<uint8_t, S * CHUNK_BYTES> keyBytes{};
    in.read(reinterpret_cast<char *>(keyBytes.data()), keyBytes.size());
    if (in.gcount() != static_cast<std::streamsize>(keyBytes.size()))
      throw std::runtime_error("Failed to read entire key");

    std::array<chunk, S> keyChunks{};
    for (size_t i = 0; i < keyChunks.size(); ++i) {
      const size_t j = i * 4;

      keyChunks[i] = (static_cast<uint32_t>(keyBytes[j]) << 24) |
                     (static_cast<uint32_t>(keyBytes[j + 1]) << 16) |
                     (static_cast<uint32_t>(keyBytes[j + 2]) << 8) |
                     static_cast<uint32_t>(keyBytes[j + 3]);
    }

    keys.push_back(K(keyChunks));
  }

  in.close();
  return keys;
}

std::vector<std::array<chunk, BLOCK_SIZE>>
readBlocksFromFile(std::filesystem::path path) {
  std::ifstream in;
  in.open(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("Could not open blocks file");

  std::vector<std::array<chunk, BLOCK_SIZE>> plaintexts = {};

  while (in.peek() != EOF) {
    std::array<uint8_t, BLOCK_SIZE_BYTES> plaintextBytes{};
    in.read(reinterpret_cast<char *>(plaintextBytes.data()),
            plaintextBytes.size());
    if (in.gcount() != static_cast<std::streamsize>(plaintextBytes.size()))
      throw std::runtime_error("Failed to read entire block");

    std::array<chunk, BLOCK_SIZE> block{};
    for (size_t i = 0; i < block.size(); ++i) {
      const size_t j = i * 4;

      block[i] = (static_cast<uint32_t>(plaintextBytes[j]) << 24) |
                 (static_cast<uint32_t>(plaintextBytes[j + 1]) << 16) |
                 (static_cast<uint32_t>(plaintextBytes[j + 2]) << 8) |
                 static_cast<uint32_t>(plaintextBytes[j + 3]);
    }

    plaintexts.push_back(block);
  }

  in.close();
  return plaintexts;
}

/* Current state:
 * [ARIA-128] x 1 000 000: ~1.5 s with -O3, 1 minute otherwise
 * [ARIA-256] x 1 000 000: ~1.75 s with -O3
 *
 * Goals:
 * ~~[ARIA-128] x 1 000 000: 10 seconds with -O3~~
 * Check that all outputs are correct by using the same test data with existing
 * C# implementation. Also allow persisting the keys for fast repeated
 * encryption / decryption with the same master key (more of a real use case)
 */
#define PRECISE_TIMING false

template <Derived<AriaKey> K>
void test(const std::vector<K> &keys,
          const std::vector<std::array<chunk, BLOCK_SIZE>> &blocks,
          long printReps) {
  const long reps = std::min(keys.size(), blocks.size());
  std::array<chunk, BLOCK_SIZE> encrypted;
  std::array<chunk, BLOCK_SIZE> decrypted;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < reps; i++) {
    // printAsHex("Key: ", keys[i].getValue());
    // printAsHex("Plaintext: ", blocks[i]);
#if PRECISE_TIMING
    auto beforeCrypt = std::chrono::high_resolution_clock::now();
#endif
    encrypt(blocks[i], keys[i], encrypted);
    // printAsHex("Encrypted: ", encrypted);
#if PRECISE_TIMING
    auto afterEncrypt = std::chrono::high_resolution_clock::now();
#endif
    decrypt(encrypted, keys[i], decrypted);
#if PRECISE_TIMING
    auto afterCrypt = std::chrono::high_resolution_clock::now();
#endif
    // printAsHex("Decrypted: ", decrypted);
    // if (!blocksEqual(block, decrypted)) {
    //   std::cerr << std::format("Plaintext != decrypted (iteration {})", i)
    //             << std::endl;
    //   printAsHex("Key: ", keys[i].getValue());
    //   printAsHex("Plaintext: ", blocks[i]);
    //   printAsHex("Encrypted: ", encrypted);
    //   printAsHex("Decrypted: ", decrypted);
    //   return;
    // }
#if PRECISE_TIMING
    auto afterAll = std::chrono::high_resolution_clock::now();
    std::print("Total: {}\n - Cryptography: {}\n   of "
               "which: Encryption: {}, "
               "Decryption: {}\n",
               afterAll - beforeCrypt, afterCrypt - beforeCrypt,
               afterEncrypt - beforeCrypt, afterCrypt - afterEncrypt);
#endif

    if (printReps != 0 && i % printReps == 0 && i != 0) {
      std::cout << "i: " << std::dec << i << std::endl;
    }
  }
  auto duration = std::chrono::high_resolution_clock::now() - start;
  std::print("Completed {} iterations in {:%H:%M:%S}\n", reps, duration);
}

int main() {
  unsigned int testCount = 1000000;
  const char *keyPath = "keys.txt";
  const char *plaintextPath = "texts.txt";

  writeKeysToFile<AriaKey128, AriaKey128::keySizeBits / CHUNK_BITS>(keyPath,
                                                                    testCount);
  writeBlocksToFile(plaintextPath, testCount);
  auto keys =
      readKeysFromFile<AriaKey128, AriaKey128::keySizeBits / CHUNK_BITS>(
          keyPath);
  auto blocks = readBlocksFromFile(plaintextPath);
  // Only test if keys were successfully written and read
  assert(keys.size() == testCount);
  assert(blocks.size() == testCount);

  test<AriaKey128>(keys, blocks, 0);
  return 0;
}

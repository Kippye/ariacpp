#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <random>

#include "aria.h"
#include "util.h"

using namespace ariacpp;

template <class T, class U>
concept Derived = std::is_base_of<U, T>::value;

std::string bytesToString(const std::array<uint8_t, BLOCK_SIZE_CHUNKS> bytes) {
  std::string str;
  str.assign(reinterpret_cast<std::string::value_type*>(const_cast<std::uint8_t*>(bytes.data())),
             bytes.size() / sizeof(std::string::value_type));
  return str;
}

template <size_t S>
void getRandomBytes(std::array<uint8_t, S>& block) {
  static thread_local std::mt19937 rng{std::random_device{}()};

  int offset = 0;
  uint32_t bits = 0;
  for (uint8_t& d : block) {
    if (offset == 0)
      bits = rng();
    d = static_cast<uint8_t>(bits & 0xFF);
    bits >>= 8;
    if (++offset >= 4)
      offset = 0;
  }
}

template <size_t S>
void getRandomBlock(std::array<chunk, S>& block) {
  static thread_local std::mt19937 rng{std::random_device{}()};

  for (int i = 0; i < S; i++) {
    block[i] = (chunk)rng();
  }
}

template <Derived<AriaKey> K>
K getRandomKey() {
  std::array<uint8_t, K::keySizeBits / 8> data = {};

  getRandomBytes(data);

  return K(data);
}

template <Derived<AriaKey> K>
void writeKeysToFile(std::filesystem::path path, unsigned int amount) {
  std::ofstream out;
  out.open(path, std::ios::binary);
  if (!out)
    throw std::runtime_error("Could not open key file");

  for (unsigned int i = 0; i < amount; i++) {
    const AriaKey& key = getRandomKey<K>();
    const std::span<const uint8_t> keyValue = key.getValue();

    out.write(reinterpret_cast<const char*>(keyValue.data()), keyValue.size());
  }
  out.close();
}

void writeBlocksToFile(std::filesystem::path path, unsigned int amount) {
  std::ofstream out;
  out.open(path, std::ios::binary);
  if (!out)
    throw std::runtime_error("Could not open blocks file");

  for (unsigned int i = 0; i < amount; i++) {
    std::array<uint8_t, BLOCK_SIZE> block{};
    getRandomBytes(block);

    out.write(reinterpret_cast<const char*>(block.data()), sizeof(block));
  }
  out.close();
}

template <Derived<AriaKey> K>
std::vector<K> readKeysFromFile(std::filesystem::path path) {
  std::ifstream in;
  in.open(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("Could not open key file");

  std::vector<K> keys = {};

  while (in.peek() != EOF) {
    std::array<uint8_t, K::keySizeBytes> key{};
    in.read(reinterpret_cast<char*>(key.data()), key.size());
    if (in.gcount() != static_cast<std::streamsize>(key.size()))
      throw std::runtime_error("Failed to read entire key");

    keys.push_back(K(key));
  }

  in.close();
  return keys;
}

std::vector<std::array<uint8_t, BLOCK_SIZE>> readBlocksFromFile(std::filesystem::path path) {
  std::ifstream in;
  in.open(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("Could not open blocks file");

  std::vector<std::array<uint8_t, BLOCK_SIZE>> plaintexts = {};

  while (in.peek() != EOF) {
    std::array<uint8_t, BLOCK_SIZE> block{};
    in.read(reinterpret_cast<char*>(block.data()), block.size());
    if (in.gcount() != static_cast<std::streamsize>(block.size())) {
      throw std::runtime_error("Failed to read entire block");
    }

    plaintexts.push_back(block);
  }

  in.close();
  return plaintexts;
}

/* Current state:
 * Random keys (encrypt + decrypt)
 *  [ARIA-128] x 1 000 000: ~1.3s
 *  [ARIA-192] x 1 000 000: ~1.4s
 *  [ARIA-256] x 1 000 000: ~1.6s
 * Set key (encrypt + decrypt)
 *  [ARIA-128] x 1 000 000: ~700ms
 *  [ARIA-192] x 1 000 000: ~800ms
 *  [ARIA-256] x 1 000 000: ~900ms
 *
 * Goals:
 *
 */
#define PRECISE_TIMING false
const char* keyPath = "keys.txt";
const char* plaintextPath = "blocks.txt";
const long printReps = 0;

void test(const AriaKey& key, const std::vector<std::array<uint8_t, ariacpp::BLOCK_SIZE>>& blocks) {
  const long reps = blocks.size();
  std::array<uint8_t, ariacpp::BLOCK_SIZE> encrypted;
  std::array<uint8_t, ariacpp::BLOCK_SIZE> decrypted;
  auto start = std::chrono::high_resolution_clock::now();
  Aria aria(key, true);
  // printAsHex("Key: ", key.getValue());
  for (int i = 0; i < reps; i++) {
    // printAsHex("Plaintext: ", blocks[i]);
#if PRECISE_TIMING
    auto beforeCrypt = std::chrono::high_resolution_clock::now();
#endif
    aria.encrypt(blocks[i], encrypted);
    // printAsHex("Encrypted: ", encrypted);
#if PRECISE_TIMING
    auto afterEncrypt = std::chrono::high_resolution_clock::now();
#endif
    aria.decrypt(encrypted, decrypted);
    // printAsHex("Decrypted: ", decrypted);
#if PRECISE_TIMING
    auto afterCrypt = std::chrono::high_resolution_clock::now();
#endif
#if PRECISE_TIMING
    auto afterAll = std::chrono::high_resolution_clock::now();
    std::print("Total: {}\n - Cryptography: {}\n   of "
               "which: Encryption: {}, "
               "Decryption: {}\n",
               afterAll - beforeCrypt, afterCrypt - beforeCrypt, afterEncrypt - beforeCrypt,
               afterCrypt - afterEncrypt);
#endif

    if (printReps != 0 && i % printReps == 0 && i != 0) {
      std::cout << "i: " << std::dec << i << std::endl;
    }
  }
  auto duration = std::chrono::high_resolution_clock::now() - start;
  std::print("Completed {} iterations in {:%H:%M:%S}\n", reps, duration);
}

template <Derived<AriaKey> K>
void test(const std::vector<K>& keys, const std::vector<std::array<uint8_t, ariacpp::BLOCK_SIZE>>& blocks) {
  const long reps = std::min(keys.size(), blocks.size());
  std::array<uint8_t, ariacpp::BLOCK_SIZE> encrypted;
  std::array<uint8_t, ariacpp::BLOCK_SIZE> decrypted;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < reps; i++) {
    // printAsHex("Key: ", keys[i].getValue());
    Aria aria(keys[i], true);
    // printAsHex("Plaintext: ", blocks[i]);
#if PRECISE_TIMING
    auto beforeCrypt = std::chrono::high_resolution_clock::now();
#endif
    aria.encrypt(blocks[i], encrypted);
    // printAsHex("Encrypted: ", encrypted);
#if PRECISE_TIMING
    auto afterEncrypt = std::chrono::high_resolution_clock::now();
#endif
    aria.decrypt(encrypted, decrypted);
    // printAsHex("Decrypted: ", decrypted);
#if PRECISE_TIMING
    auto afterCrypt = std::chrono::high_resolution_clock::now();
#endif
#if PRECISE_TIMING
    auto afterAll = std::chrono::high_resolution_clock::now();
    std::print("Total: {}\n - Cryptography: {}\n   of "
               "which: Encryption: {}, "
               "Decryption: {}\n",
               afterAll - beforeCrypt, afterCrypt - beforeCrypt, afterEncrypt - beforeCrypt,
               afterCrypt - afterEncrypt);
#endif

    if (printReps != 0 && i % printReps == 0 && i != 0) {
      std::cout << "i: " << std::dec << i << std::endl;
    }
  }
  auto duration = std::chrono::high_resolution_clock::now() - start;
  std::print("Completed {} iterations in {:%H:%M:%S}\n", reps, duration);
}

template <Derived<AriaKey> K>
void testFromFile(unsigned int testCount) {
  writeKeysToFile<K>(keyPath, testCount);
  writeBlocksToFile(plaintextPath, testCount);
  auto keys = readKeysFromFile<K>(keyPath);
  auto blocks = readBlocksFromFile(plaintextPath);

  assert(keys.size() == testCount);
  assert(blocks.size() == testCount);

  test<K>(keys, blocks);
}

void testWithKey(AriaKey& key, unsigned int testCount) {
  writeBlocksToFile(plaintextPath, testCount);
  auto blocks = readBlocksFromFile(plaintextPath);

  assert(blocks.size() == testCount);

  test(key, blocks);
}

int main() {
  auto key = getRandomKey<AriaKey256>();

  testWithKey(key, 1000000);

  // testFromFile<AriaKey256>(1000000);
  return 0;
}

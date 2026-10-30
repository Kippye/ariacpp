#include "aria.h"
#include <chrono>
#include <iostream>
#include <print>
#include <random>

template <class T, class U>
concept Derived = std::is_base_of<U, T>::value;

template <std::size_t S>
bool blocksEqual(const std::array<uint8_t, S> &a,
                 const std::array<uint8_t, S> &b) {
  for (int i = 0; i < S; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void printAsHex(const char *label, const std::span<const uint8_t> &input) {
  std::cout << label;
  for (int i = 0; i < input.size(); i++) {
    std::cout << std::hex << (int)input[i] << " ";
  }
  std::cout << std::endl;
}

std::string bytesToString(const std::array<uint8_t, BLOCK_SIZE> bytes) {
  std::string str;
  str.assign(reinterpret_cast<std::string::value_type *>(
                 const_cast<std::uint8_t *>(bytes.data())),
             bytes.size() / sizeof(std::string::value_type));
  return str;
}

std::random_device rd;
std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

// For now, just 128-bit keys
template <Derived<AriaKey> K, size_t S> K getRandomKey() {
  std::array<uint8_t, S> data = {};

  int offset = 0;
  uint32_t bits = 0;
  for (uint8_t &d : data) {
    if (offset == 0)
      bits = dist(rd);
    d = static_cast<uint8_t>(bits & 0xFF);
    bits >>= 8;
    if (++offset >= 4)
      offset = 0;
  }

  return K(data);
}

void getRandomBlock(std::array<uint8_t, 16> &block) {
  int offset = 0;
  uint32_t bits = 0;
  for (uint8_t &d : block) {
    if (offset == 0)
      bits = dist(rd);
    d = static_cast<uint8_t>(bits & 0xFF);
    bits >>= 8;
    if (++offset >= 4)
      offset = 0;
  }
}

/* Current state:
 * [ARIA-128] x 1 000 000: ~20 seconds with -O3, 1 minute otherwise
 * ~20% of testing time spent on generating random keys and blocks
 *
 * Goals:
 * [ARIA-128] x 1 000 000: 10 seconds with -O3
 * Also allow persisting the keys for fast repeated encryption / decryption
 * with the same master key (more of a real use case)
 */
#define PRECISE_TIMING false

void test(long reps, long printReps) {
  std::array<uint8_t, 16> block;
  std::array<uint8_t, 16> encrypted;
  std::array<uint8_t, 16> decrypted;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < reps; i++) {
#if PRECISE_TIMING
    auto beforeGen = std::chrono::high_resolution_clock::now();
#endif
    const AriaKey &key = getRandomKey<AriaKey128, 16>();
    getRandomBlock(block);
#if PRECISE_TIMING
    auto afterGen = std::chrono::high_resolution_clock::now();
#endif

    // printAsHex("Key: ", key.getValue());
    // printAsHex("Plaintext: ", block);

#if PRECISE_TIMING
    auto beforeCrypt = std::chrono::high_resolution_clock::now();
#endif
    encrypt(block, key, encrypted);
// printAsHex("Encrypted: ", encrypted);
#if PRECISE_TIMING
    auto afterEncrypt = std::chrono::high_resolution_clock::now();
#endif
    decrypt(encrypted, key, decrypted);
#if PRECISE_TIMING
    auto afterCrypt = std::chrono::high_resolution_clock::now();
#endif
    // printAsHex("Decrypted: ", decrypted);
    if (!blocksEqual(block, decrypted)) {
      std::cerr << std::format("Plaintext != decrypted (iteration {})", i)
                << std::endl;
      printAsHex("Key: ", key.getValue());
      printAsHex("Plaintext: ", block);
      printAsHex("Encrypted: ", encrypted);
      printAsHex("Decrypted: ", decrypted);
      return;
    }
    auto afterAll = std::chrono::high_resolution_clock::now();

#if PRECISE_TIMING
    std::print("Total: {}\n - Generation: {}\n - Cryptography: {}\n   of "
               "which: Encryption: {}, "
               "Decryption: {}\n",
               afterAll - beforeGen, afterGen - beforeGen,
               afterCrypt - beforeCrypt, afterEncrypt - beforeCrypt,
               afterCrypt - afterEncrypt);
#endif

    if (i % printReps == 0 && i != 0) {
      std::cout << "i: " << std::dec << i << std::endl;
    }
  }
  auto duration = std::chrono::high_resolution_clock::now() - start;
  std::print("Completed {} iterations in {:%H:%M:%S}\n", reps, duration);
}

int main() {
  test(1000000, 100000);
  return 0;
}

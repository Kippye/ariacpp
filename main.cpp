#include "aria.h"
#include "util.h"
#include <chrono>
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

void getRandomBlock(std::array<chunk, BLOCK_SIZE> &block) {
  static thread_local std::mt19937 rng{std::random_device{}()};

  for (int i = 0; i < BLOCK_SIZE; i++) {
    block[i] = (chunk)rng();
  }
}

template <Derived<AriaKey> K, size_t S> K getRandomKey() {
  std::array<chunk, S> data = {};

  getRandomBlock<S>(data);

  return K(data);
}

/* Current state:
 * [ARIA-128] x 1 000 000: ~1.5 s with -O3, 1 minute otherwise
 * [ARIA-256] x 1 000 000: ~1.75 s with -O3
 * Test data generation overhead negligible (~2% of encrypt + decrypt time)
 *
 * Goals:
 * ~~[ARIA-128] x 1 000 000: 10 seconds with -O3~~
 * Also allow persisting the keys for fast repeated encryption / decryption
 * with the same master key (more of a real use case)
 */
#define PRECISE_TIMING false

void test(long reps, long printReps) {
  std::array<chunk, BLOCK_SIZE> block;
  std::array<chunk, BLOCK_SIZE> encrypted;
  std::array<chunk, BLOCK_SIZE> decrypted;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < reps; i++) {
#if PRECISE_TIMING
    auto beforeGen = std::chrono::high_resolution_clock::now();
#endif
    // const AriaKey &key =
    //     AriaKey128({0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f}); //
    //     Official test vector key
    const AriaKey &key =
        getRandomKey<AriaKey256, AriaKey256::keySizeBits / CHUNK_BITS>();
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
    // if (!blocksEqual(block, decrypted)) {
    //   std::cerr << std::format("Plaintext != decrypted (iteration {})", i)
    //             << std::endl;
    //   printAsHex("Key: ", key.getValue());
    //   printAsHex("Plaintext: ", block);
    //   printAsHex("Encrypted: ", encrypted);
    //   printAsHex("Decrypted: ", decrypted);
    //   return;
    // }
#if PRECISE_TIMING
    auto afterAll = std::chrono::high_resolution_clock::now();
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

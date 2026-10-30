#if !defined(ARIA_CPP)
#define ARIA_CPP 1

#include <array>
#include <cstdint>
#include <span>

// Size of a block in bytes
constexpr size_t BLOCK_SIZE = 16;

class AriaKey {
public:
  virtual size_t getSize() const = 0;
  virtual std::span<const uint8_t> getValue() const = 0;
};

class AriaKey128 : public AriaKey {
private:
  std::array<uint8_t, 16> _key;

public:
  AriaKey128(const std::array<uint8_t, 16> &key) { _key = key; }

  size_t getSize() const override { return _key.size(); }
  std::span<const uint8_t> getValue() const override { return _key; }
};

class AriaKey192 : public AriaKey {
private:
  std::array<uint8_t, 24> _key;

public:
  AriaKey192(const std::array<uint8_t, 24> &key) { _key = key; }

  size_t getSize() const override { return _key.size(); }
  std::span<const uint8_t> getValue() const override { return _key; }
};

class AriaKey256 : public AriaKey {
private:
  std::array<uint8_t, 32> _key;

public:
  AriaKey256(const std::array<uint8_t, 32> &key) { _key = key; }

  size_t getSize() const override { return _key.size(); }
  std::span<const uint8_t> getValue() const override { return _key; }
};

/// @brief Encrypt a block using a key.
/// @param plaintext
/// @param key
/// @return Encrypted block
std::array<uint8_t, BLOCK_SIZE>
encrypt(const std::array<uint8_t, BLOCK_SIZE> &plaintext, const AriaKey &key);

/// @brief Encrypt a block into output using a key.
/// @param plaintext
/// @param key
/// @param output
void encrypt(const std::array<uint8_t, BLOCK_SIZE> &plaintext,
             const AriaKey &key, std::array<uint8_t, BLOCK_SIZE> &output);

/// @brief Decrypt a ciphertext block using a key.
/// @param ciphertext
/// @param key
/// @return Decrypted block
std::array<uint8_t, BLOCK_SIZE>
decrypt(const std::array<uint8_t, BLOCK_SIZE> &ciphertext, const AriaKey &key);

/// @brief Decrypt a ciphertext block into output using a key.
/// @param ciphertext
/// @param key
/// @param output
void decrypt(const std::array<uint8_t, BLOCK_SIZE> &ciphertext,
             const AriaKey &key, std::array<uint8_t, BLOCK_SIZE> &output);
#endif
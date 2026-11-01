#if !defined(ARIA_CPP)
#define ARIA_CPP 1

#include <array>
#include <cstdint>
#include <span>

// Size of a block in bytes
constexpr size_t BLOCK_SIZE_BYTES = 16;
using chunk = uint32_t;
constexpr size_t CHUNK_BYTES = sizeof(chunk);
constexpr size_t CHUNK_BITS = sizeof(chunk) * 8;
// Size of a block in chunks
constexpr size_t BLOCK_SIZE = BLOCK_SIZE_BYTES / CHUNK_BYTES;

class AriaKey {
  public:
    // Get the size of the key in chunks
    virtual size_t getSizeChunks() const = 0;
    // Get the size of the key in bits
    virtual size_t getSizeBits() const = 0;
    virtual std::span<const chunk> getValue() const = 0;
};

class AriaKey128 : public AriaKey {
  private:
    std::array<chunk, 128 / CHUNK_BITS> _key;

  public:
    static constexpr size_t keySizeBits = 128;
    AriaKey128(const std::array<chunk, 128 / CHUNK_BITS>& key) {
      _key = key;
    }

    size_t getSizeChunks() const override {
      return _key.size();
    }
    size_t getSizeBits() const override {
      return keySizeBits;
    };
    std::span<const chunk> getValue() const override {
      return _key;
    }
};

class AriaKey192 : public AriaKey {
  private:
    std::array<chunk, 192 / CHUNK_BITS> _key;

  public:
    static constexpr size_t keySizeBits = 192;
    AriaKey192(const std::array<chunk, 192 / CHUNK_BITS>& key) {
      _key = key;
    }

    size_t getSizeChunks() const override {
      return _key.size();
    }
    size_t getSizeBits() const override {
      return keySizeBits;
    };
    std::span<const chunk> getValue() const override {
      return _key;
    }
};

class AriaKey256 : public AriaKey {
  private:
    std::array<chunk, 256 / CHUNK_BITS> _key;

  public:
    static constexpr size_t keySizeBits = 256;
    AriaKey256(const std::array<chunk, 256 / CHUNK_BITS>& key) {
      _key = key;
    }

    size_t getSizeChunks() const override {
      return _key.size();
    }
    size_t getSizeBits() const override {
      return keySizeBits;
    };
    std::span<const chunk> getValue() const override {
      return _key;
    }
};

/// @brief Encrypt a block of bytes using a key.
/// @param plaintext
/// @param key
/// @return Encrypted block
std::array<uint8_t, BLOCK_SIZE_BYTES> encrypt(const std::array<uint8_t, BLOCK_SIZE_BYTES>& plaintext,
                                              const AriaKey& key);

/// @brief Encrypt a block into output using a key.
/// @param plaintext
/// @param key
/// @param output
void encrypt(const std::array<chunk, BLOCK_SIZE>& plaintext,
             const AriaKey& key,
             std::array<chunk, BLOCK_SIZE>& output);

/// @brief Decrypt a ciphertext block of bytes using a key.
/// @param ciphertext
/// @param key
/// @return Decrypted block
std::array<uint8_t, BLOCK_SIZE_BYTES> decrypt(const std::array<uint8_t, BLOCK_SIZE_BYTES>& ciphertext,
                                              const AriaKey& key);

/// @brief Decrypt a ciphertext block into output using a key.
/// @param ciphertext
/// @param key
/// @param output
void decrypt(const std::array<chunk, BLOCK_SIZE>& ciphertext,
             const AriaKey& key,
             std::array<chunk, BLOCK_SIZE>& output);
#endif
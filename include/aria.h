#ifndef ARIA_CPP
#define ARIA_CPP 1

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace ariacpp {
  using chunk = uint32_t;
  // Size of a block in bytes
  constexpr size_t BLOCK_SIZE = 16;
  // Size of a chunk in bytes
  constexpr size_t CHUNK_BYTES = sizeof(chunk);
  // Size of a chunk in bits
  constexpr size_t CHUNK_BITS = sizeof(chunk) * 8;
  // Size of a block in chunks
  constexpr size_t BLOCK_SIZE_CHUNKS = BLOCK_SIZE / CHUNK_BYTES;

  class AriaKey {
      friend class Aria;

    private:
      // Get the size of the key in chunks
      virtual size_t getSizeChunks() const = 0;

    public:
      // Get the size of the key in bits
      virtual size_t getSize() const = 0;
      virtual std::span<const uint8_t> getValue() const = 0;
  };

  class AriaKey128 : public AriaKey {
      friend class Aria;

    private:
      std::array<uint8_t, 16> _key;
      size_t getSizeChunks() const override {
        return _key.size() / CHUNK_BYTES;
      }

    public:
      static constexpr size_t keySizeBits = 128;
      static constexpr size_t keySizeBytes = 16;
      AriaKey128(const std::array<uint8_t, 16>& key) {
        _key = key;
      }

      size_t getSize() const override {
        return keySizeBits;
      };
      std::span<const uint8_t> getValue() const override {
        return _key;
      }
  };

  class AriaKey192 : public AriaKey {
      friend class Aria;

    private:
      std::array<uint8_t, 24> _key;
      size_t getSizeChunks() const override {
        return _key.size() / CHUNK_BYTES;
      }

    public:
      static constexpr size_t keySizeBits = 192;
      static constexpr size_t keySizeBytes = 24;
      AriaKey192(const std::array<uint8_t, 24>& key) {
        _key = key;
      }

      size_t getSize() const override {
        return keySizeBits;
      };
      std::span<const uint8_t> getValue() const override {
        return _key;
      }
  };

  class AriaKey256 : public AriaKey {
      friend class Aria;

    private:
      std::array<uint8_t, 32> _key;
      size_t getSizeChunks() const override {
        return _key.size() / CHUNK_BYTES;
      }

    public:
      static constexpr size_t keySizeBits = 256;
      static constexpr size_t keySizeBytes = 32;
      AriaKey256(const std::array<uint8_t, 32>& key) {
        _key = key;
      }

      size_t getSize() const override {
        return keySizeBits;
      };
      std::span<const uint8_t> getValue() const override {
        return _key;
      }
  };

  class Aria {
    private:
      std::vector<std::array<chunk, BLOCK_SIZE_CHUNKS>> _encryptionKeys;
      bool _hasEncryptionKeys = false;
      std::vector<std::array<chunk, BLOCK_SIZE_CHUNKS>> _decryptionKeys;
      bool _hasDecryptionKeys = false;

      // Create encryption round keys based on the master key.
      void createEncryptionKeys(const AriaKey& key);
      // Create decryption round keys based on the encryption keys, throw if the instance doesn't have any.
      void createDecryptionKeys();

      /// @brief Encrypt a block into output.
      /// @param plaintext
      /// @param output
      void encrypt(const std::array<chunk, BLOCK_SIZE_CHUNKS>& plaintext,
                   std::array<chunk, BLOCK_SIZE_CHUNKS>& output) const;

      /// @brief Decrypt a ciphertext block into output.
      /// @param ciphertext
      /// @param key
      /// @param output
      void decrypt(const std::array<chunk, BLOCK_SIZE_CHUNKS>& ciphertext,
                   std::array<chunk, BLOCK_SIZE_CHUNKS>& output) const;

    public:
      // Create an instance of the ARIA cipher and create round keys.
      // Encryption round keys will always be created.
      // Decryption round keys will only be generated if enableDecryption is true.
      // NOTE: The master key itself will not be stored.
      Aria(const AriaKey& key, bool enableDecryption);

      // Returns true if this instance is set up to decrypt.
      bool isAbleToDecrypt() const;

      /// @brief Encrypt a block of bytes.
      /// @param plaintext
      /// @return Encrypted block
      std::array<uint8_t, BLOCK_SIZE> encrypt(const std::array<uint8_t, BLOCK_SIZE>& plaintext) const;

      /// @brief Encrypt a block of bytes into output.
      /// @param plaintext
      /// @param output
      void encrypt(const std::array<uint8_t, BLOCK_SIZE>& plaintext,
                   std::array<uint8_t, BLOCK_SIZE>& output) const;

      /// @brief Decrypt a ciphertext block.
      /// @param ciphertext
      /// @return Decrypted block
      std::array<uint8_t, BLOCK_SIZE> decrypt(const std::array<uint8_t, BLOCK_SIZE>& ciphertext) const;

      /// @brief Decrypt a ciphertext block into output.
      /// @param plaintext
      /// @param output
      void decrypt(const std::array<uint8_t, BLOCK_SIZE>& ciphertext,
                   std::array<uint8_t, BLOCK_SIZE>& output) const;
  };
}  // namespace ariacpp
#endif
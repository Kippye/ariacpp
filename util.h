#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <span>

using chunk = uint32_t;

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

template <std::size_t S>
bool blocksEqual(const std::array<chunk, S> &a, const std::array<chunk, S> &b) {
  for (int i = 0; i < S; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

inline void printAsHex(const char *label,
                       const std::span<const uint8_t> &input) {
  std::cout << label;
  for (int i = 0; i < input.size(); i++) {
    std::cout << std::hex << (unsigned int)input[i] << " ";
  }
  std::cout << std::endl;
}

inline void printAsHex(const char *label, const std::span<const chunk> &input) {
  std::cout << label;
  for (int i = 0; i < input.size(); i++) {
    std::cout << std::hex << (unsigned int)input[i] << " ";
  }
  std::cout << std::endl;
}
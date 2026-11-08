#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace symenc::protocol {

using TokenBytes = std::array<std::uint8_t, 3>;

inline constexpr std::size_t kTokenLength = 3;

inline constexpr TokenBytes kJig{{0x4A, 0x49, 0x47}};
inline constexpr TokenBytes kSaw{{0x53, 0x41, 0x57}};
inline constexpr TokenBytes kEuro{{0xE2, 0x82, 0xAC}};

inline constexpr std::size_t kJigWeight = 50;
inline constexpr std::size_t kMaxByteValue = 127;
inline constexpr std::size_t kMaxJig = 2;
inline constexpr std::size_t kMaxEuro = 49;
inline constexpr std::size_t kMaxEuroAtMaxJig = 27;

constexpr bool is_whitespace(std::uint8_t byte) noexcept {
  return byte == 0x20 || byte == 0x09 || byte == 0x0A || byte == 0x0D;
}

constexpr std::size_t group_token_count(std::size_t value) noexcept {
  return value / kJigWeight + value % kJigWeight + 1;
}

namespace detail {
constexpr std::size_t max_group_tokens() noexcept {
  std::size_t most = 0;
  for (std::size_t value = 0; value <= kMaxByteValue; ++value) {
    const std::size_t tokens = group_token_count(value);
    most = tokens > most ? tokens : most;
  }
  return most;
}
}

inline constexpr std::size_t kMaxGroupBytes = kTokenLength * detail::max_group_tokens();

static_assert(kMaxGroupBytes == 153, "spec section 14.1: worst case is 153 bytes");
static_assert(kMaxJig * kJigWeight + kMaxEuroAtMaxJig == kMaxByteValue,
              "j = 2 admits e <= 27 exactly");
static_assert(kMaxJig * (kMaxEuro + 1) + (kMaxEuroAtMaxJig + 1) == kMaxByteValue + 1,
              "spec section 3: the grammar admits exactly 128 groups (50 + 50 + 28)");

}

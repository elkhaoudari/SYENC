#pragma once

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

#include "symenc/codec.hpp"

namespace symtest {

inline constexpr std::string_view kJigText{"JIG"};
inline constexpr std::string_view kSawText{"SAW"};
inline constexpr std::string_view kEuroText{"\xE2\x82\xAC", 3};

inline std::string text_of(std::initializer_list<unsigned> values) {
  std::string text;
  for (const unsigned value : values) text.push_back(static_cast<char>(value));
  return text;
}

inline symenc::Bytes bytes_of(std::initializer_list<unsigned> values) {
  symenc::Bytes bytes;
  for (const unsigned value : values) bytes.push_back(static_cast<std::uint8_t>(value));
  return bytes;
}

inline symenc::Bytes bytes_of_text(std::string_view text) {
  symenc::Bytes bytes;
  for (const char c : text) bytes.push_back(static_cast<std::uint8_t>(c));
  return bytes;
}

inline std::string repeat(std::string_view piece, std::size_t times) {
  std::string out;
  out.reserve(piece.size() * times);
  for (std::size_t i = 0; i < times; ++i) out.append(piece);
  return out;
}

inline std::string hex(std::string_view data) {
  static const char digits[] = "0123456789ABCDEF";
  std::string out;
  for (const char c : data) {
    const unsigned byte = static_cast<unsigned char>(c);
    if (!out.empty()) out.push_back(' ');
    out.push_back(digits[byte >> 4]);
    out.push_back(digits[byte & 0x0F]);
  }
  return out;
}

inline std::string hex_of(const symenc::Bytes& bytes) {
  std::string raw;
  raw.reserve(bytes.size());
  for (const std::uint8_t b : bytes) raw.push_back(static_cast<char>(b));
  return hex(raw);
}

inline std::size_t count_of(std::string_view text, std::string_view piece) {
  std::size_t count = 0;
  for (std::size_t at = text.find(piece); at != std::string_view::npos;
       at = text.find(piece, at + piece.size())) {
    ++count;
  }
  return count;
}

inline std::string group(std::size_t jigs, std::size_t euros) {
  return repeat(kJigText, jigs) + repeat(kEuroText, euros) + std::string(kSawText);
}

}

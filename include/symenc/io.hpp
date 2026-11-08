#pragma once

#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string_view>

#include "symenc/codec.hpp"

namespace symenc {

struct IoError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

[[nodiscard]] Bytes read_binary(const std::filesystem::path& file);

[[nodiscard]] Bytes read_binary(std::istream& in);

void write_binary_atomic(const std::filesystem::path& dest, std::string_view data);

void write_binary(std::ostream& out, std::string_view data);

[[nodiscard]] inline std::string_view as_text(const Bytes& bytes) noexcept {
  return std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

}

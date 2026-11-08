#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "symenc/codec.hpp"
#include "symenc/protocol.hpp"

namespace symenc {

namespace {

void put(std::string& out, std::size_t& at, const protocol::TokenBytes& token) {
  for (const std::uint8_t byte : token) out[at++] = static_cast<char>(byte);
}

}

Result<std::string> encode(const Bytes& source) {
  const std::size_t limit = std::string().max_size();
  std::size_t total = 0;
  for (std::size_t k = 0; k < source.size(); ++k) {
    const std::size_t value = source[k];
    if (value > protocol::kMaxByteValue) return Error{ErrorCode::NonAsciiInput, k};
    const std::size_t group_bytes = protocol::kTokenLength * protocol::group_token_count(value);
    if (total > limit - group_bytes) throw std::length_error("symenc::encode: output too large");
    total += group_bytes;
  }

  std::string out(total, '\0');
  std::size_t at = 0;
  for (const std::uint8_t byte : source) {
    const std::size_t value = byte;
    for (std::size_t i = 0; i < value / protocol::kJigWeight; ++i) put(out, at, protocol::kJig);
    for (std::size_t i = 0; i < value % protocol::kJigWeight; ++i) put(out, at, protocol::kEuro);
    put(out, at, protocol::kSaw);
  }
  assert(at == total);
  return Result<std::string>(std::move(out));
}

}

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "symenc/error.hpp"

namespace symenc {

using Bytes = std::vector<std::uint8_t>;

[[nodiscard]] Result<std::string> encode(const Bytes& source);

[[nodiscard]] Result<Bytes> decode(std::string_view text);

}

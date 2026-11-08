#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "symenc/error.hpp"

namespace symenc {

enum class TokenKind : std::uint8_t { Jig, Saw, Euro };

[[nodiscard]] std::string_view name(TokenKind kind) noexcept;

struct Token {
  TokenKind kind;
  std::size_t offset;
};

struct Lex {
  enum class Status : std::uint8_t {
    Token,
    End,
    Failed,
  };
  Status status;
  Token token;
  Error error;
};

[[nodiscard]] Lex next_token(std::string_view text, std::size_t& pos) noexcept;

}

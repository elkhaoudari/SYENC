#include "symenc/tokenizer.hpp"

#include "symenc/protocol.hpp"

namespace symenc {

std::string_view name(TokenKind kind) noexcept {
  switch (kind) {
    case TokenKind::Jig:  return "JIG";
    case TokenKind::Saw:  return "SAW";
    case TokenKind::Euro: return "EURO";
  }
  return "UNKNOWN";
}

namespace {

Lex make_token(TokenKind kind, std::size_t offset) noexcept {
  return Lex{Lex::Status::Token, Token{kind, offset}, Error{ErrorCode::BadToken, 0}};
}

Lex make_end() noexcept {
  return Lex{Lex::Status::End, Token{TokenKind::Jig, 0}, Error{ErrorCode::BadToken, 0}};
}

Lex make_failure(ErrorCode code, std::size_t offset) noexcept {
  return Lex{Lex::Status::Failed, Token{TokenKind::Jig, 0}, Error{code, offset}};
}

std::uint8_t byte_at(std::string_view text, std::size_t index) noexcept {
  return static_cast<std::uint8_t>(text[index]);
}

}

Lex next_token(std::string_view text, std::size_t& pos) noexcept {
  const std::size_t n = text.size();
  std::size_t p = pos > n ? n : pos;

  while (p < n && protocol::is_whitespace(byte_at(text, p))) ++p;
  if (p == n) {
    pos = n;
    return make_end();
  }

  const std::size_t start = p;
  const std::uint8_t first = byte_at(text, start);
  TokenKind kind;
  const protocol::TokenBytes* pattern;
  if (first == protocol::kJig[0]) {
    kind = TokenKind::Jig;
    pattern = &protocol::kJig;
  } else if (first == protocol::kSaw[0]) {
    kind = TokenKind::Saw;
    pattern = &protocol::kSaw;
  } else if (first == protocol::kEuro[0]) {
    kind = TokenKind::Euro;
    pattern = &protocol::kEuro;
  } else {
    return make_failure(ErrorCode::BadToken, start);
  }

  for (std::size_t k = 1; k < protocol::kTokenLength; ++k) {
    if (start + k >= n) return make_failure(ErrorCode::Truncated, n);
    if (byte_at(text, start + k) != (*pattern)[k]) {
      return make_failure(ErrorCode::BadToken, start);
    }
  }

  pos = start + protocol::kTokenLength;
  return make_token(kind, start);
}

}

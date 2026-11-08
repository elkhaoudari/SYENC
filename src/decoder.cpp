#include <cstddef>
#include <string_view>
#include <utility>

#include "symenc/codec.hpp"
#include "symenc/protocol.hpp"
#include "symenc/tokenizer.hpp"

namespace symenc {

Result<Bytes> decode(std::string_view text) {
  Bytes out;
  std::size_t pos = 0;
  std::size_t j = 0;
  std::size_t e = 0;

  for (;;) {
    const Lex lex = next_token(text, pos);
    if (lex.status == Lex::Status::End) break;
    if (lex.status == Lex::Status::Failed) return lex.error;

    const std::size_t at = lex.token.offset;
    switch (lex.token.kind) {
      case TokenKind::Jig:
        if (e > 0) return Error{ErrorCode::Order, at};
        if (j == protocol::kMaxJig) return Error{ErrorCode::NonCanonical, at};
        ++j;
        break;
      case TokenKind::Euro:
        if (e == protocol::kMaxEuro) return Error{ErrorCode::NonCanonical, at};
        if (protocol::kJigWeight * j + e + 1 > protocol::kMaxByteValue) {
          return Error{ErrorCode::Range, at};
        }
        ++e;
        break;
      case TokenKind::Saw:
        out.push_back(static_cast<std::uint8_t>(protocol::kJigWeight * j + e));
        j = 0;
        e = 0;
        break;
    }
  }

  if (j > 0 || e > 0) return Error{ErrorCode::Truncated, text.size()};
  return Result<Bytes>(std::move(out));
}

}

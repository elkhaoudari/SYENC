#pragma once

#include <ostream>

#include "symenc/tokenizer.hpp"

namespace symenc {

inline std::ostream& operator<<(std::ostream& out, TokenKind kind) { return out << name(kind); }

inline std::ostream& operator<<(std::ostream& out, Lex::Status status) {
  switch (status) {
    case Lex::Status::Token:  return out << "Token";
    case Lex::Status::End:    return out << "End";
    case Lex::Status::Failed: return out << "Failed";
  }
  return out << "?";
}

}

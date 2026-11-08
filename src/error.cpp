#include "symenc/error.hpp"

namespace symenc {

std::string_view name(ErrorCode code) noexcept {
  switch (code) {
    case ErrorCode::NonAsciiInput: return "E_NONASCII_INPUT";
    case ErrorCode::BadToken:      return "E_BAD_TOKEN";
    case ErrorCode::Order:         return "E_ORDER";
    case ErrorCode::NonCanonical:  return "E_NONCANONICAL";
    case ErrorCode::Range:         return "E_RANGE";
    case ErrorCode::Truncated:     return "E_TRUNCATED";
  }
  return "E_UNKNOWN";
}

std::string format(const Error& error) {
  std::string text(name(error.code));
  text += " at byte offset ";
  text += std::to_string(error.offset);
  return text;
}

}

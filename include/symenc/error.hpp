#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace symenc {

enum class ErrorCode : std::uint8_t {
  NonAsciiInput,
  BadToken,
  Order,
  NonCanonical,
  Range,
  Truncated,
};

inline constexpr std::array<ErrorCode, 6> kAllErrorCodes{{
    ErrorCode::NonAsciiInput,
    ErrorCode::BadToken,
    ErrorCode::Order,
    ErrorCode::NonCanonical,
    ErrorCode::Range,
    ErrorCode::Truncated,
}};

struct Error {
  ErrorCode code;
  std::size_t offset;
};

constexpr bool operator==(const Error& a, const Error& b) noexcept {
  return a.code == b.code && a.offset == b.offset;
}
constexpr bool operator!=(const Error& a, const Error& b) noexcept { return !(a == b); }

[[nodiscard]] std::string_view name(ErrorCode code) noexcept;

[[nodiscard]] std::string format(const Error& error);

template <class T>
using Result = std::variant<T, Error>;

template <class T>
[[nodiscard]] bool failed(const Result<T>& result) noexcept {
  static_assert(!std::is_same_v<T, Error>, "Result<Error> would be ambiguous");
  return std::holds_alternative<Error>(result);
}

template <class T>
[[nodiscard]] const Error& error_of(const Result<T>& result) {
  return std::get<Error>(result);
}

template <class T>
[[nodiscard]] const T& value_of(const Result<T>& result) {
  return std::get<T>(result);
}

template <class T>
[[nodiscard]] T&& value_of(Result<T>&& result) {
  return std::get<T>(std::move(result));
}

}

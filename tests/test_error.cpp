#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "error_printers.hpp"
#include "symenc/error.hpp"
#include "test_framework.hpp"

using symenc::Error;
using symenc::ErrorCode;
using symenc::Result;

static_assert(std::is_trivially_copyable_v<Error>, "Error must stay a plain value type");

SYMENC_TEST(names_match_the_spec_exactly) {
  CHECK_EQ(symenc::name(ErrorCode::NonAsciiInput), std::string_view("E_NONASCII_INPUT"));
  CHECK_EQ(symenc::name(ErrorCode::BadToken), std::string_view("E_BAD_TOKEN"));
  CHECK_EQ(symenc::name(ErrorCode::Order), std::string_view("E_ORDER"));
  CHECK_EQ(symenc::name(ErrorCode::NonCanonical), std::string_view("E_NONCANONICAL"));
  CHECK_EQ(symenc::name(ErrorCode::Range), std::string_view("E_RANGE"));
  CHECK_EQ(symenc::name(ErrorCode::Truncated), std::string_view("E_TRUNCATED"));
}

SYMENC_TEST(there_are_exactly_six_distinct_names) {
  std::set<std::string_view> seen;
  for (ErrorCode code : symenc::kAllErrorCodes) {
    CHECK(symenc::name(code).substr(0, 2) == "E_");
    seen.insert(symenc::name(code));
  }
  CHECK_EQ(seen.size(), std::size_t{6});
  CHECK_EQ(symenc::kAllErrorCodes.size(), std::size_t{6});
}

SYMENC_TEST(an_invalid_code_value_has_a_safe_name) {
  CHECK_EQ(symenc::name(static_cast<ErrorCode>(200)), std::string_view("E_UNKNOWN"));
}

SYMENC_TEST(errors_compare_by_code_and_offset) {
  const Error a{ErrorCode::BadToken, 3};
  CHECK(a == Error({ErrorCode::BadToken, 3}));
  CHECK(a != Error({ErrorCode::BadToken, 4}));
  CHECK(a != Error({ErrorCode::Order, 3}));
}

SYMENC_TEST(format_follows_the_recommended_message) {
  CHECK_EQ(symenc::format({ErrorCode::BadToken, 3}), std::string("E_BAD_TOKEN at byte offset 3"));
  CHECK_EQ(symenc::format({ErrorCode::Truncated, 6}), std::string("E_TRUNCATED at byte offset 6"));
  CHECK_EQ(symenc::format({ErrorCode::NonAsciiInput, 0}), std::string("E_NONASCII_INPUT at byte offset 0"));
  CHECK_EQ(symenc::format({ErrorCode::Range, 1048576}), std::string("E_RANGE at byte offset 1048576"));
}

SYMENC_TEST(result_holds_a_value) {
  const Result<std::string> r = std::string("JIGSAW");
  CHECK(!symenc::failed(r));
  CHECK_EQ(symenc::value_of(r), std::string("JIGSAW"));
}

SYMENC_TEST(result_holds_an_error) {
  const Result<std::vector<std::uint8_t>> r = Error{ErrorCode::Order, 3};
  CHECK(symenc::failed(r));
  CHECK_EQ(symenc::error_of(r), Error({ErrorCode::Order, 3}));
}

SYMENC_TEST(wrong_accessor_throws_instead_of_misbehaving) {
  const Result<std::string> ok = std::string("x");
  const Result<std::string> bad = Error{ErrorCode::Range, 87};
  bool threw_on_error_of_value = false;
  bool threw_on_value_of_error = false;
  try { static_cast<void>(symenc::error_of(ok)); } catch (const std::bad_variant_access&) { threw_on_error_of_value = true; }
  try { static_cast<void>(symenc::value_of(bad)); } catch (const std::bad_variant_access&) { threw_on_value_of_error = true; }
  CHECK(threw_on_error_of_value);
  CHECK(threw_on_value_of_error);
}

SYMENC_TEST(value_can_be_moved_out_of_an_rvalue_result) {
  Result<std::string> r = std::string(1000, 'x');
  const std::string moved = symenc::value_of(std::move(r));
  CHECK_EQ(moved.size(), std::size_t{1000});
}

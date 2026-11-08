#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "error_printers.hpp"
#include "symenc/codec.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"

using symenc::Bytes;
using symenc::Error;
using symenc::ErrorCode;
using symtest::bytes_of;
using symtest::group;
using symtest::hex_of;
using symtest::kEuroText;
using symtest::kJigText;
using symtest::kSawText;
using symtest::repeat;
using symtest::text_of;

namespace {

const std::string EURO(kEuroText);
const std::string JIG(kJigText);
const std::string SAW(kSawText);

std::string euros(std::size_t n) { return repeat(kEuroText, n); }
std::string jigs(std::size_t n) { return repeat(kJigText, n); }

void expect_bytes(const std::string& text, const Bytes& expected) {
  const auto result = symenc::decode(text);
  CHECK(!symenc::failed(result));
  if (!symenc::failed(result)) CHECK_EQ(hex_of(symenc::value_of(result)), hex_of(expected));
}

void expect_error(const std::string& text, ErrorCode code, std::size_t offset) {
  const auto result = symenc::decode(text);
  CHECK(symenc::failed(result));
  if (symenc::failed(result)) CHECK_EQ(symenc::error_of(result), Error({code, offset}));
}

}

SYMENC_TEST(E01_to_E16_decode_back_to_their_sources) {
  expect_bytes("", bytes_of({}));
  expect_bytes(group(0, 0), bytes_of({0x00}));
  expect_bytes(group(0, 9), bytes_of({0x09}));
  expect_bytes(group(0, 10), bytes_of({0x0A}));
  expect_bytes(group(0, 13) + group(0, 10), bytes_of({0x0D, 0x0A}));
  expect_bytes(group(0, 32), bytes_of({0x20}));
  expect_bytes(group(1, 15), bytes_of({0x41}));
  expect_bytes(group(1, 22), bytes_of({0x48}));
  expect_bytes(group(2, 5), bytes_of({0x69}));
  expect_bytes(group(2, 8), bytes_of({0x6C}));
  expect_bytes(group(2, 0), bytes_of({0x64}));
  expect_bytes(group(2, 27), bytes_of({0x7F}));
  expect_bytes(group(0, 49), bytes_of({0x31}));
  expect_bytes(group(1, 0), bytes_of({0x32}));
  expect_bytes(group(1, 49), bytes_of({0x63}));
}

SYMENC_TEST(JIGSAW_decodes_to_the_single_byte_0x32) {
  expect_bytes("JIGSAW", bytes_of({0x32}));
}

SYMENC_TEST(boundary_values_0_49_50_99_100_127) {
  expect_bytes(group(0, 0) + group(0, 49) + group(1, 0) + group(1, 49) + group(2, 0) + group(2, 27),
               bytes_of({0, 49, 50, 99, 100, 127}));
}

SYMENC_TEST(W1_to_W4_whitespace_is_accepted_between_tokens) {
  expect_bytes("JIG\r\n" + EURO + EURO + " SAW\n", bytes_of({0x34}));
  expect_bytes("   ", bytes_of({}));
  expect_bytes("SAW\tSAW", bytes_of({0x00, 0x00}));
  expect_bytes("\nJIGSAW", bytes_of({0x32}));
  expect_bytes(" \t\r\n", bytes_of({}));
}

SYMENC_TEST(case_01_fiftieth_euro_is_noncanonical) {
  expect_error(euros(50) + SAW, ErrorCode::NonCanonical, 147);
}
SYMENC_TEST(case_02_third_jig_is_noncanonical_not_range) {
  expect_error(jigs(3) + SAW, ErrorCode::NonCanonical, 6);
}
SYMENC_TEST(case_03_jig_after_euro_is_order) {
  expect_error(EURO + JIG + SAW, ErrorCode::Order, 3);
}
SYMENC_TEST(case_04_twenty_eighth_euro_after_two_jigs_is_range) {
  expect_error(jigs(2) + euros(28) + SAW, ErrorCode::Range, 87);
}
SYMENC_TEST(case_05_missing_final_saw_is_truncated_at_input_length) {
  expect_error(euros(2), ErrorCode::Truncated, 6);
}
SYMENC_TEST(case_06_unknown_byte_after_a_token) {
  expect_error("JIGXSAW", ErrorCode::BadToken, 3);
}
SYMENC_TEST(case_07_invalid_utf8_byte_is_bad_token) {
  expect_error(text_of({0xFF}), ErrorCode::BadToken, 0);
}
SYMENC_TEST(case_08_euro_prefix_at_end_of_input_is_truncated) {
  expect_error(text_of({0xE2, 0x82}), ErrorCode::Truncated, 2);
}
SYMENC_TEST(case_09_wrong_third_euro_byte_is_bad_token_at_token_start) {
  expect_error(text_of({0xE2, 0x82, 0xAD}) + SAW, ErrorCode::BadToken, 0);
}
SYMENC_TEST(case_10_wrong_second_euro_byte_is_bad_token) {
  expect_error(text_of({0xE2, 0x28, 0xA1}), ErrorCode::BadToken, 0);
}
SYMENC_TEST(case_11_valid_utf8_that_is_not_a_token) {
  expect_error(text_of({0xC3, 0xA9}) + SAW, ErrorCode::BadToken, 0);
}
SYMENC_TEST(case_12_earlier_order_error_beats_a_later_range_view) {
  expect_error(jigs(2) + euros(27) + JIG + SAW, ErrorCode::Order, 87);
}
SYMENC_TEST(case_13_SAWS_is_a_token_then_a_truncated_prefix) {
  expect_error("SAWS", ErrorCode::Truncated, 4);
}
SYMENC_TEST(case_14_jig_without_saw_is_truncated) {
  expect_error("JIG", ErrorCode::Truncated, 3);
}
SYMENC_TEST(case_15_third_byte_mismatch) {
  expect_error("JIX", ErrorCode::BadToken, 0);
}

SYMENC_TEST(N1_to_N9_additional_error_vectors) {
  expect_error("jig" + SAW, ErrorCode::BadToken, 0);
  expect_error(JIG + text_of({0xC2, 0xA0}) + SAW, ErrorCode::BadToken, 3);
  expect_error(text_of({0xEF, 0xBB, 0xBF}) + SAW, ErrorCode::BadToken, 0);
  expect_error(text_of({0x00}), ErrorCode::BadToken, 0);
  expect_error("J IG SAW", ErrorCode::BadToken, 0);
  for (const auto& bad : {text_of({0x80}), text_of({0xC0, 0xAF}), text_of({0xED, 0xA0, 0x80}),
                          text_of({0xF0, 0x9F, 0x98, 0x80})}) {
    expect_error(bad, ErrorCode::BadToken, 0);
  }
  expect_error(SAW + text_of({0x80}), ErrorCode::BadToken, 3);
  expect_error(SAW + EURO, ErrorCode::Truncated, 6);
  expect_error(jigs(2) + euros(28) + JIG + SAW, ErrorCode::Range, 87);
}

SYMENC_TEST(offsets_count_ignored_whitespace) {
  expect_error("\n X", ErrorCode::BadToken, 2);
  expect_error(EURO + "\n" + JIG + SAW, ErrorCode::Order, 4);
  expect_error(" \n" + EURO + " " + JIG + " " + SAW, ErrorCode::Order, 6);
  expect_error(EURO + "  ", ErrorCode::Truncated, 5);
  expect_error(SAW + text_of({0xC2, 0xA0}), ErrorCode::BadToken, 3);
}

SYMENC_TEST(state_IDLE) {
  expect_bytes(SAW, bytes_of({0}));
  expect_error(JIG, ErrorCode::Truncated, 3);
  expect_error(EURO, ErrorCode::Truncated, 3);
  expect_bytes("", bytes_of({}));
}

SYMENC_TEST(state_JIGS_with_one_jig) {
  expect_bytes(JIG + SAW, bytes_of({50}));
  expect_bytes(JIG + JIG + SAW, bytes_of({100}));
  expect_bytes(JIG + EURO + SAW, bytes_of({51}));
  expect_error(JIG, ErrorCode::Truncated, 3);
}

SYMENC_TEST(state_JIGS_with_two_jigs) {
  expect_error(jigs(3) + SAW, ErrorCode::NonCanonical, 6);
  expect_bytes(jigs(2) + EURO + SAW, bytes_of({101}));
  expect_bytes(jigs(2) + SAW, bytes_of({100}));
  expect_error(jigs(2), ErrorCode::Truncated, 6);
}

SYMENC_TEST(state_UNITS) {
  expect_error(EURO + JIG, ErrorCode::Order, 3);
  expect_error(JIG + EURO + JIG, ErrorCode::Order, 6);
  expect_bytes(EURO + EURO + SAW, bytes_of({2}));
  expect_bytes(euros(7) + SAW, bytes_of({7}));
  expect_error(EURO, ErrorCode::Truncated, 3);
}

SYMENC_TEST(jig_is_rejected_after_a_euro_until_the_next_group_begins) {
  expect_error(EURO + JIG + SAW, ErrorCode::Order, 3);
  expect_bytes(EURO + SAW + JIG + SAW, bytes_of({1, 50}));
  expect_bytes(EURO + SAW + JIG + EURO + SAW, bytes_of({1, 51}));
}

SYMENC_TEST(the_largest_valid_group_in_each_jig_class) {
  expect_bytes(euros(49) + SAW, bytes_of({49}));
  expect_bytes(JIG + euros(49) + SAW, bytes_of({99}));
  expect_bytes(jigs(2) + euros(27) + SAW, bytes_of({127}));
}

SYMENC_TEST(the_first_token_past_each_limit_fails_at_its_own_offset) {
  expect_error(euros(50), ErrorCode::NonCanonical, 147);
  expect_error(JIG + euros(50), ErrorCode::NonCanonical, 3 + 147);
  expect_error(jigs(2) + euros(28), ErrorCode::Range, 6 + 81);
}

SYMENC_TEST(error_is_raised_before_end_of_input_when_both_apply) {
  expect_error(jigs(2) + euros(28), ErrorCode::Range, 87);
  expect_error(euros(50), ErrorCode::NonCanonical, 147);
}

SYMENC_TEST(a_lexical_error_after_a_syntax_error_is_never_reported) {
  expect_error(EURO + JIG + "X", ErrorCode::Order, 3);
}

SYMENC_TEST(a_syntax_error_after_a_lexical_error_is_never_reported) {
  expect_error("X" + EURO + JIG, ErrorCode::BadToken, 0);
}

SYMENC_TEST(failures_return_only_the_error_never_decoded_bytes) {
  const auto result = symenc::decode(group(1, 15) + "X");
  CHECK(symenc::failed(result));
  if (symenc::failed(result)) CHECK_EQ(symenc::error_of(result), Error({ErrorCode::BadToken, 51}));
}

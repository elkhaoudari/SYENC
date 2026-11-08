#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "error_printers.hpp"
#include "symenc/codec.hpp"
#include "symenc/protocol.hpp"
#include "symenc/tokenizer.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "token_printers.hpp"

using symenc::Bytes;
using symenc::Error;
using symenc::ErrorCode;
using symtest::bytes_of;
using symtest::group;
using symtest::hex;
using symtest::repeat;

namespace {

std::string encode_ok(const Bytes& source) {
  const auto result = symenc::encode(source);
  CHECK(!symenc::failed(result));
  if (symenc::failed(result)) return {};
  return symenc::value_of(result);
}

void expect_error(const Bytes& source, std::size_t offset) {
  const auto result = symenc::encode(source);
  CHECK(symenc::failed(result));
  if (symenc::failed(result)) {
    CHECK_EQ(symenc::error_of(result), Error({ErrorCode::NonAsciiInput, offset}));
  }
}

struct Vector {
  const char* id;
  Bytes source;
  std::string expected;
  std::size_t expected_bytes;
};

}

SYMENC_TEST(spec_encoding_vectors_E01_to_E16) {
  const std::vector<Vector> vectors = {
      {"E01 empty", bytes_of({}), "", 0},
      {"E02 NUL", bytes_of({0x00}), group(0, 0), 3},
      {"E03 TAB", bytes_of({0x09}), group(0, 9), 30},
      {"E04 LF", bytes_of({0x0A}), group(0, 10), 33},
      {"E05 CRLF", bytes_of({0x0D, 0x0A}), group(0, 13) + group(0, 10), 75},
      {"E06 space", bytes_of({0x20}), group(0, 32), 99},
      {"E07 A", bytes_of({0x41}), group(1, 15), 51},
      {"E08 H", bytes_of({0x48}), group(1, 22), 72},
      {"E09 i", bytes_of({0x69}), group(2, 5), 24},
      {"E10 l", bytes_of({0x6C}), group(2, 8), 33},
      {"E11 d", bytes_of({0x64}), group(2, 0), 9},
      {"E12 DEL", bytes_of({0x7F}), group(2, 27), 90},
      {"E13 N=49", bytes_of({0x31}), group(0, 49), 150},
      {"E14 N=50", bytes_of({0x32}), group(1, 0), 6},
      {"E15 N=99", bytes_of({0x63}), group(1, 49), 153},
      {"E16 N=100", bytes_of({0x64}), group(2, 0), 9},
  };
  for (const Vector& v : vectors) {
    const std::string encoded = encode_ok(v.source);
    CHECK_EQ(encoded, v.expected);
    CHECK_EQ(encoded.size(), v.expected_bytes);
    CHECK_EQ(v.expected.size(), v.expected_bytes);
  }
}

SYMENC_TEST(spec_byte_level_references) {
  CHECK_EQ(hex(encode_ok(bytes_of({0x00}))), std::string("53 41 57"));
  CHECK_EQ(hex(encode_ok(bytes_of({0x01}))), std::string("E2 82 AC 53 41 57"));
  CHECK_EQ(hex(encode_ok(bytes_of({0x32}))), std::string("4A 49 47 53 41 57"));
  CHECK_EQ(hex(encode_ok(bytes_of({0x64}))), std::string("4A 49 47 4A 49 47 53 41 57"));
}

SYMENC_TEST(group_sizes_at_the_boundary_values) {
  const std::vector<std::pair<unsigned, std::size_t>> cases = {
      {0, 3}, {49, 150}, {50, 6}, {99, 153}, {100, 9}, {127, 90}};
  for (const auto& [value, bytes] : cases) {
    CHECK_EQ(encode_ok(bytes_of({value})).size(), bytes);
  }
}

SYMENC_TEST(spec_encoder_error_vectors_X1_to_X5) {
  expect_error(bytes_of({0x80}), 0);
  expect_error(bytes_of({0x41, 0x42, 0x80}), 2);
  expect_error(bytes_of({0xC3, 0xA9}), 0);
  Bytes big(1048576, 0x61);
  big.push_back(0xFF);
  expect_error(big, 1048576);
  expect_error(bytes_of({0xEF, 0xBB, 0xBF, 0x78}), 0);
}

SYMENC_TEST(the_first_offending_byte_wins_and_valid_prefixes_do_not_matter) {
  expect_error(bytes_of({0x5A, 0x80, 0xFF}), 1);
  expect_error(bytes_of({0x00, 0x7F, 0x00, 0x7F, 0x81}), 4);
}

SYMENC_TEST(output_is_deterministic) {
  const Bytes source = bytes_of({'H', 'i', '\n', 0x00, 0x7F});
  CHECK_EQ(encode_ok(source), encode_ok(source));
}

SYMENC_TEST(output_contains_only_back_to_back_tokens) {
  const std::string message = "Hello, World!\n\t\r\n";
  const std::string encoded = encode_ok(symtest::bytes_of_text(message));

  std::size_t expected_tokens = 0;
  for (const char c : message) {
    const std::size_t value = static_cast<unsigned char>(c);
    expected_tokens += value / 50 + value % 50 + 1;
  }

  std::size_t pos = 0;
  std::size_t tokens = 0;
  bool contiguous = true;
  for (;;) {
    const symenc::Lex lex = symenc::next_token(encoded, pos);
    if (lex.status != symenc::Lex::Status::Token) {
      CHECK_EQ(lex.status, symenc::Lex::Status::End);
      break;
    }
    contiguous = contiguous && lex.token.offset == 3 * tokens;
    ++tokens;
  }
  CHECK(contiguous);
  CHECK_EQ(tokens, expected_tokens);
  CHECK_EQ(encoded.size(), 3 * expected_tokens);
}

SYMENC_TEST(line_endings_are_never_normalized) {
  CHECK_EQ(encode_ok(bytes_of({0x0D, 0x0A})), group(0, 13) + group(0, 10));
  CHECK_EQ(encode_ok(bytes_of({0x0A})), group(0, 10));
  CHECK_EQ(encode_ok(bytes_of({0x0D})), group(0, 13));
  CHECK(encode_ok(bytes_of({0x0D, 0x0A})) != encode_ok(bytes_of({0x0A})));
}

SYMENC_TEST(a_large_worst_case_input_encodes_to_the_exact_size) {
  const std::size_t count = 100000;
  const Bytes source(count, 0x63);
  const std::string encoded = encode_ok(source);
  CHECK_EQ(encoded.size(), count * symenc::protocol::kMaxGroupBytes);
  CHECK_EQ(encoded.substr(0, 153), group(1, 49));
  CHECK_EQ(encoded.substr(encoded.size() - 153), group(1, 49));
}

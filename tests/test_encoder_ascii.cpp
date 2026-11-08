#include <cstddef>
#include <string>
#include <vector>

#include "error_printers.hpp"
#include "symenc/codec.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"

using symenc::Bytes;
using symtest::bytes_of;
using symtest::count_of;
using symtest::group;
using symtest::kEuroText;
using symtest::kJigText;
using symtest::kSawText;

namespace {

std::string encode_ok(const Bytes& source) {
  const auto result = symenc::encode(source);
  CHECK(!symenc::failed(result));
  return symenc::failed(result) ? std::string() : symenc::value_of(result);
}

std::string encode_value(unsigned value) { return encode_ok(bytes_of({value})); }

}

SYMENC_TEST(all_128_values_match_the_spec_formula) {
  for (unsigned n = 0; n <= 127; ++n) {
    const std::string encoded = encode_value(n);
    CHECK_EQ(encoded, group(n / 50, n % 50));
    CHECK_EQ(encoded.size(), std::size_t{3} * (n / 50 + n % 50 + 1));
  }
}

SYMENC_TEST(all_128_values_have_canonical_token_counts) {
  for (unsigned n = 0; n <= 127; ++n) {
    const std::string encoded = encode_value(n);
    const std::size_t jigs = count_of(encoded, kJigText);
    const std::size_t euros = count_of(encoded, kEuroText);
    CHECK_EQ(count_of(encoded, kSawText), std::size_t{1});
    CHECK_EQ(encoded.substr(encoded.size() - 3), std::string("SAW"));
    CHECK(jigs <= 2);
    CHECK(euros <= 49);
    CHECK_EQ(50 * jigs + euros, std::size_t{n});
    if (jigs == 2) CHECK(euros <= 27);
  }
}

SYMENC_TEST(the_three_jig_classes_split_at_50_and_100) {
  for (unsigned n = 0; n <= 127; ++n) {
    const std::size_t jigs = count_of(encode_value(n), kJigText);
    CHECK_EQ(jigs, std::size_t{n < 50 ? 0u : (n < 100 ? 1u : 2u)});
  }
}

SYMENC_TEST(the_128_encodings_are_distinct_and_prefix_free) {
  std::vector<std::string> all;
  for (unsigned n = 0; n <= 127; ++n) all.push_back(encode_value(n));
  bool distinct = true;
  bool prefix_free = true;
  for (std::size_t a = 0; a < all.size(); ++a) {
    for (std::size_t b = 0; b < all.size(); ++b) {
      if (a == b) continue;
      distinct = distinct && all[a] != all[b];
      prefix_free = prefix_free && all[b].compare(0, all[a].size(), all[a]) != 0;
    }
  }
  CHECK(distinct);
  CHECK(prefix_free);
}

SYMENC_TEST(the_encoding_of_a_string_is_the_concatenation_of_its_groups) {
  Bytes all;
  std::string expected;
  for (unsigned n = 0; n <= 127; ++n) {
    all.push_back(static_cast<std::uint8_t>(n));
    expected += encode_value(n);
  }
  CHECK_EQ(encode_ok(all), expected);
}

SYMENC_TEST(R2_R3_all_values_ascending_and_descending_take_9186_bytes) {
  Bytes ascending;
  Bytes descending;
  for (unsigned n = 0; n <= 127; ++n) {
    ascending.push_back(static_cast<std::uint8_t>(n));
    descending.push_back(static_cast<std::uint8_t>(127 - n));
  }
  CHECK_EQ(encode_ok(ascending).size(), std::size_t{9186});
  CHECK_EQ(encode_ok(descending).size(), std::size_t{9186});
}

SYMENC_TEST(R4_boundary_values_in_sequence) {
  const std::string encoded = encode_ok(bytes_of({0x00, 0x31, 0x32, 0x63, 0x64, 0x7F}));
  CHECK_EQ(encoded, group(0, 0) + group(0, 49) + group(1, 0) + group(1, 49) + group(2, 0) + group(2, 27));
  CHECK_EQ(encoded.size(), std::size_t{411});
  CHECK_EQ(encoded.size() / 3, std::size_t{137});
}

SYMENC_TEST(R5_a_thousand_NUL_bytes_are_a_thousand_SAW_tokens) {
  const std::string encoded = encode_ok(Bytes(1000, 0x00));
  CHECK_EQ(encoded.size(), std::size_t{3000});
  CHECK_EQ(encoded, symtest::repeat(kSawText, 1000));
}

SYMENC_TEST(R6_line_ending_variants_encode_to_pairwise_distinct_texts) {
  const std::vector<Bytes> variants = {
      bytes_of({0x0A}),
      bytes_of({0x0D, 0x0A}),
      bytes_of({0x0D}),
      bytes_of({0x0A, 0x0D}),
      bytes_of({0x61, 0x0D, 0x0A, 0x62}),
      bytes_of({0x61, 0x0A}),
  };
  std::vector<std::string> encoded;
  for (const Bytes& v : variants) encoded.push_back(encode_ok(v));
  bool distinct = true;
  for (std::size_t a = 0; a < encoded.size(); ++a)
    for (std::size_t b = a + 1; b < encoded.size(); ++b) distinct = distinct && encoded[a] != encoded[b];
  CHECK(distinct);
}

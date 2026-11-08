#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <variant>
#include <vector>

#include "error_printers.hpp"
#include "symenc/codec.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"

using symenc::Bytes;
using symenc::Error;
using symenc::ErrorCode;
using symtest::bytes_of;
using symtest::hex_of;

namespace {

symenc::Result<Bytes> reference_decode(const std::string& in) {
  Bytes out;
  const std::size_t n = in.size();
  std::size_t p = 0, j = 0, e = 0;
  const auto b = [&](std::size_t i) { return static_cast<unsigned>(static_cast<unsigned char>(in[i])); };
  for (;;) {
    while (p < n && (b(p) == 0x20 || b(p) == 0x09 || b(p) == 0x0A || b(p) == 0x0D)) ++p;
    if (p == n) break;
    const std::size_t start = p;
    unsigned pattern[3];
    int tok;
    if (b(p) == 0x4A)      { tok = 0; pattern[0] = 0x4A; pattern[1] = 0x49; pattern[2] = 0x47; }
    else if (b(p) == 0xE2) { tok = 1; pattern[0] = 0xE2; pattern[1] = 0x82; pattern[2] = 0xAC; }
    else if (b(p) == 0x53) { tok = 2; pattern[0] = 0x53; pattern[1] = 0x41; pattern[2] = 0x57; }
    else return Error{ErrorCode::BadToken, start};
    for (std::size_t k = 1; k <= 2; ++k) {
      if (start + k >= n) return Error{ErrorCode::Truncated, n};
      if (b(start + k) != pattern[k]) return Error{ErrorCode::BadToken, start};
    }
    p = start + 3;
    if (tok == 0) {
      if (e > 0) return Error{ErrorCode::Order, start};
      if (j == 2) return Error{ErrorCode::NonCanonical, start};
      j += 1;
    } else if (tok == 1) {
      if (e == 49) return Error{ErrorCode::NonCanonical, start};
      if (50 * j + e + 1 > 127) return Error{ErrorCode::Range, start};
      e += 1;
    } else {
      out.push_back(static_cast<std::uint8_t>(50 * j + e));
      j = 0;
      e = 0;
    }
  }
  if (j > 0 || e > 0) return Error{ErrorCode::Truncated, n};
  return out;
}

struct Rng {
  std::mt19937_64 engine;
  explicit Rng(std::uint64_t seed) : engine(seed) {}
  std::size_t below(std::size_t bound) { return static_cast<std::size_t>(engine() % bound); }
};

Bytes random_ascii(Rng& rng, std::size_t length) {
  Bytes bytes(length);
  for (auto& byte : bytes) byte = static_cast<std::uint8_t>(rng.below(128));
  return bytes;
}

std::string encode_ok(const Bytes& source) {
  const auto result = symenc::encode(source);
  CHECK(!symenc::failed(result));
  return symenc::failed(result) ? std::string() : symenc::value_of(result);
}

bool round_trips(const Bytes& source) {
  const std::string encoded = encode_ok(source);
  const auto decoded = symenc::decode(encoded);
  return !symenc::failed(decoded) && symenc::value_of(decoded) == source;
}

bool same_outcome(const symenc::Result<Bytes>& a, const symenc::Result<Bytes>& b) {
  if (symenc::failed(a) != symenc::failed(b)) return false;
  if (symenc::failed(a)) return symenc::error_of(a) == symenc::error_of(b);
  return symenc::value_of(a) == symenc::value_of(b);
}

std::string strip_whitespace(const std::string& text) {
  std::string out;
  for (const char c : text) {
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') out.push_back(c);
  }
  return out;
}

}

SYMENC_TEST(R1_every_single_byte_round_trips) {
  for (unsigned n = 0; n <= 127; ++n) CHECK(round_trips(bytes_of({n})));
}

SYMENC_TEST(R2_R3_R4_R5_fixed_inputs_round_trip) {
  Bytes ascending, descending;
  for (unsigned n = 0; n <= 127; ++n) {
    ascending.push_back(static_cast<std::uint8_t>(n));
    descending.push_back(static_cast<std::uint8_t>(127 - n));
  }
  CHECK(round_trips(ascending));
  CHECK(round_trips(descending));
  CHECK(round_trips(bytes_of({0x00, 0x31, 0x32, 0x63, 0x64, 0x7F})));
  CHECK(round_trips(Bytes(1000, 0x00)));
  CHECK(round_trips(Bytes{}));
}

SYMENC_TEST(R6_line_endings_survive_exactly) {
  const std::vector<Bytes> variants = {
      bytes_of({0x0A}), bytes_of({0x0D, 0x0A}), bytes_of({0x0D}), bytes_of({0x0A, 0x0D}),
      bytes_of({0x61, 0x0D, 0x0A, 0x62}), bytes_of({0x61, 0x0A}), bytes_of({0x09, 0x20, 0x7F, 0x00})};
  for (const Bytes& v : variants) CHECK(round_trips(v));
}

SYMENC_TEST(R7_ten_thousand_random_inputs_up_to_4096_bytes) {
  Rng rng(0xC0FFEE);
  std::size_t failures = 0;
  for (int i = 0; i < 10000; ++i) {
    const Bytes source = random_ascii(rng, rng.below(4097));
    if (!round_trips(source)) ++failures;
  }
  CHECK_EQ(failures, std::size_t{0});
}

SYMENC_TEST(R7_five_one_mebibyte_inputs) {
  Rng rng(0xC0FFEE + 1);
  for (int i = 0; i < 5; ++i) CHECK(round_trips(random_ascii(rng, 1048576)));
}

SYMENC_TEST(R8_encode_decode_encode_is_idempotent) {
  Rng rng(0xBEEF);
  for (int i = 0; i < 500; ++i) {
    const Bytes source = random_ascii(rng, rng.below(300));
    const std::string once = encode_ok(source);
    const auto decoded = symenc::decode(once);
    CHECK(!symenc::failed(decoded));
    if (!symenc::failed(decoded)) CHECK(encode_ok(symenc::value_of(decoded)) == once);
  }
}

SYMENC_TEST(R9_random_whitespace_between_tokens_never_changes_the_result) {
  Rng rng(0xFACE);
  const char ws[4] = {' ', '\t', '\n', '\r'};
  for (int i = 0; i < 300; ++i) {
    const Bytes source = random_ascii(rng, rng.below(200));
    const std::string encoded = encode_ok(source);
    std::string spaced;
    for (std::size_t at = 0; at < encoded.size(); at += 3) {
      for (std::size_t run = rng.below(3); run > 0; --run) spaced.push_back(ws[rng.below(4)]);
      spaced.append(encoded, at, 3);
    }
    for (std::size_t run = rng.below(4); run > 0; --run) spaced.push_back(ws[rng.below(4)]);
    const auto decoded = symenc::decode(spaced);
    CHECK(!symenc::failed(decoded));
    if (!symenc::failed(decoded)) CHECK(symenc::value_of(decoded) == source);
  }
}

SYMENC_TEST(F1_every_proper_prefix_of_a_valid_encoding_is_truncated_or_a_group_boundary) {
  Rng rng(0x1234);
  std::vector<Bytes> sources;
  for (unsigned n = 0; n <= 127; ++n) sources.push_back(bytes_of({n}));
  for (int i = 0; i < 300; ++i) sources.push_back(random_ascii(rng, rng.below(9)));
  for (const Bytes& source : sources) {
    const std::string encoded = encode_ok(source);
    std::vector<std::size_t> boundary_after_bytes(encoded.size() + 1, static_cast<std::size_t>(-1));
    std::size_t at = 0;
    boundary_after_bytes[0] = 0;
    for (std::size_t i = 0; i < source.size(); ++i) {
      at += 3 * (source[i] / 50u + source[i] % 50u + 1u);
      boundary_after_bytes[at] = i + 1;
    }
    for (std::size_t k = 0; k <= encoded.size(); ++k) {
      const auto result = symenc::decode(encoded.substr(0, k));
      if (boundary_after_bytes[k] != static_cast<std::size_t>(-1)) {
        CHECK(!symenc::failed(result));
        if (!symenc::failed(result)) {
          CHECK(symenc::value_of(result) == Bytes(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(boundary_after_bytes[k])));
        }
      } else {
        CHECK(symenc::failed(result));
        if (symenc::failed(result)) CHECK_EQ(symenc::error_of(result), Error({ErrorCode::Truncated, k}));
      }
    }
  }
}

namespace {

std::string random_soup(Rng& rng) {
  static const std::string pieces[] = {"JIG", "SAW", "\xE2\x82\xAC"};
  std::string text;
  for (std::size_t count = rng.below(40); count > 0; --count) {
    const std::size_t kind = rng.below(20);
    if (kind < 12) {
      text += pieces[rng.below(3)];
    } else if (kind < 14) {
      text += ' ';
    } else if (kind < 15) {
      text += '\n';
    } else if (kind < 17) {
      const std::string& piece = pieces[rng.below(3)];
      text += piece.substr(0, 1 + rng.below(2));
    } else if (kind < 18) {
      text += static_cast<char>(rng.below(256));
    } else {
      for (std::size_t run = 1 + rng.below(30); run > 0; --run) text += "\xE2\x82\xAC";
    }
  }
  return text;
}

}

SYMENC_TEST(F2_random_soup_decoder_agrees_with_the_reference_decoder) {
  Rng rng(0xF00D);
  std::size_t disagreements = 0;
  std::size_t successes = 0;
  for (int i = 0; i < 200000; ++i) {
    const std::string text = random_soup(rng);
    const auto got = symenc::decode(text);
    const auto want = reference_decode(text);
    if (!same_outcome(got, want)) ++disagreements;
    if (!symenc::failed(got)) {
      ++successes;
      if (encode_ok(symenc::value_of(got)) != strip_whitespace(text)) ++disagreements;
    } else {
      const Error& error = symenc::error_of(got);
      if (error.offset > text.size() || error.code == ErrorCode::NonAsciiInput) ++disagreements;
    }
  }
  CHECK_EQ(disagreements, std::size_t{0});
  CHECK(successes > 1000);
}

SYMENC_TEST(F2_purely_random_bytes_agree_with_the_reference_decoder) {
  Rng rng(0xABCD);
  std::size_t disagreements = 0;
  for (int i = 0; i < 50000; ++i) {
    std::string text;
    for (std::size_t n = rng.below(24); n > 0; --n) text.push_back(static_cast<char>(rng.below(256)));
    if (!same_outcome(symenc::decode(text), reference_decode(text))) ++disagreements;
  }
  CHECK_EQ(disagreements, std::size_t{0});
}

SYMENC_TEST(F3_every_single_byte_mutation_of_valid_encodings) {
  Rng rng(0x7777);
  std::size_t disagreements = 0;
  std::size_t mutations = 0;
  for (int i = 0; i < 25; ++i) {
    const std::string encoded = encode_ok(random_ascii(rng, 1 + rng.below(4)));
    for (std::size_t at = 0; at < encoded.size(); ++at) {
      for (unsigned value = 0; value <= 255; ++value) {
        std::string mutated = encoded;
        mutated[at] = static_cast<char>(value);
        ++mutations;
        const auto got = symenc::decode(mutated);
        if (!same_outcome(got, reference_decode(mutated))) ++disagreements;
        if (symenc::failed(got) && symenc::error_of(got).offset > mutated.size()) ++disagreements;
      }
    }
  }
  CHECK_EQ(disagreements, std::size_t{0});
  CHECK(mutations > 10000);
}

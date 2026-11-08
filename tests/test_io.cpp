#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include "symenc/io.hpp"
#include "temp_dir.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"

namespace fs = std::filesystem;
using symenc::Bytes;
using symenc::IoError;
using symtest::TempDir;

namespace {

std::string as_string(const Bytes& bytes) { return std::string(symenc::as_text(bytes)); }

std::size_t entries_in(const fs::path& dir) {
  std::size_t count = 0;
  for (const auto& entry : fs::directory_iterator(dir)) {
    static_cast<void>(entry);
    ++count;
  }
  return count;
}

template <class F>
bool throws_io_error(F&& f) {
  try {
    f();
  } catch (const IoError&) {
    return true;
  }
  return false;
}

}

SYMENC_TEST(all_256_byte_values_survive_write_then_read) {
  TempDir dir;
  std::string data;
  for (unsigned b = 0; b <= 255; ++b) data.push_back(static_cast<char>(b));
  symenc::write_binary_atomic(dir.file("all"), data);
  CHECK_EQ(fs::file_size(dir.file("all")), std::uintmax_t{256});
  CHECK_EQ(symtest::hex(as_string(symenc::read_binary(dir.file("all")))), symtest::hex(data));
}

SYMENC_TEST(B_bytes_NUL_TAB_LF_CRLF_SPACE_DEL_are_written_verbatim) {
  TempDir dir;
  const std::vector<std::string> samples = {
      symtest::text_of({0x00}), symtest::text_of({0x09}), symtest::text_of({0x0A}),
      symtest::text_of({0x0D, 0x0A}), symtest::text_of({0x20}), symtest::text_of({0x7F})};
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const fs::path file = dir.file("f" + std::to_string(i));
    symenc::write_binary_atomic(file, samples[i]);
    CHECK_EQ(fs::file_size(file), samples[i].size());
    CHECK_EQ(symtest::hex(as_string(symenc::read_binary(file))), symtest::hex(samples[i]));
  }
}

SYMENC_TEST(an_empty_write_creates_a_zero_byte_file) {
  TempDir dir;
  symenc::write_binary_atomic(dir.file("empty"), "");
  CHECK(fs::exists(dir.file("empty")));
  CHECK_EQ(fs::file_size(dir.file("empty")), std::uintmax_t{0});
  CHECK_EQ(symenc::read_binary(dir.file("empty")).size(), std::size_t{0});
}

SYMENC_TEST(success_leaves_no_temporary_file) {
  TempDir dir;
  symenc::write_binary_atomic(dir.file("out.txt"), "hello");
  CHECK_EQ(entries_in(dir.path()), std::size_t{1});
  CHECK(fs::exists(dir.file("out.txt")));
}

SYMENC_TEST(overwriting_replaces_the_whole_old_content) {
  TempDir dir;
  symenc::write_binary_atomic(dir.file("f"), "a much longer previous content");
  symenc::write_binary_atomic(dir.file("f"), "short");
  CHECK_EQ(as_string(symenc::read_binary(dir.file("f"))), std::string("short"));
  CHECK_EQ(entries_in(dir.path()), std::size_t{1});
}

SYMENC_TEST(a_failed_replace_throws_and_leaves_destination_and_directory_untouched) {
  TempDir dir;
  const fs::path dest = dir.file("target");
  fs::create_directory(dest);
  { std::ofstream(dest / "inside.txt") << "keep"; }
  CHECK(throws_io_error([&] { symenc::write_binary_atomic(dest, "new data"); }));
  CHECK(fs::is_directory(dest));
  CHECK(fs::exists(dest / "inside.txt"));
  CHECK_EQ(entries_in(dir.path()), std::size_t{1});
}

SYMENC_TEST(writing_into_a_missing_directory_throws_and_creates_nothing) {
  TempDir dir;
  CHECK(throws_io_error([&] { symenc::write_binary_atomic(dir.file("nope/out"), "x"); }));
  CHECK_EQ(entries_in(dir.path()), std::size_t{0});
}

SYMENC_TEST(reading_a_missing_file_or_a_directory_throws) {
  TempDir dir;
  CHECK(throws_io_error([&] { static_cast<void>(symenc::read_binary(dir.file("missing"))); }));
  CHECK(throws_io_error([&] { static_cast<void>(symenc::read_binary(dir.path())); }));
}

SYMENC_TEST(files_larger_than_one_read_chunk_are_read_completely) {
  TempDir dir;
  std::string data;
  for (std::size_t i = 0; i < 200001; ++i) data.push_back(static_cast<char>(i % 251));
  symenc::write_binary_atomic(dir.file("big"), data);
  const Bytes back = symenc::read_binary(dir.file("big"));
  CHECK_EQ(back.size(), data.size());
  CHECK(as_string(back) == data);
}

SYMENC_TEST(streams_carry_NUL_and_high_bytes_unchanged) {
  const std::string data = symtest::text_of({0x00, 0xFF, 0x80, 0x0D, 0x0A, 0x1A, 0x00});
  std::istringstream in(data);
  const Bytes read = symenc::read_binary(in);
  CHECK_EQ(symtest::hex(as_string(read)), symtest::hex(data));

  std::ostringstream out;
  symenc::write_binary(out, data);
  CHECK_EQ(symtest::hex(out.str()), symtest::hex(data));
}

SYMENC_TEST(an_empty_stream_reads_as_empty_bytes) {
  std::istringstream in("");
  CHECK_EQ(symenc::read_binary(in).size(), std::size_t{0});
}

SYMENC_TEST(a_failing_output_stream_is_reported) {
  std::ostringstream out;
  out.setstate(std::ios::badbit);
  CHECK(throws_io_error([&] { symenc::write_binary(out, "x"); }));
}

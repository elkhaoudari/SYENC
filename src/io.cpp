#include "symenc/io.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <istream>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace symenc {

namespace fs = std::filesystem;

namespace {

void remove_quietly(const fs::path& file) {
  std::error_code ignored;
  fs::remove(file, ignored);
}

fs::path temp_path_for(const fs::path& dest) {
  std::random_device device;
  const std::uint64_t random = (static_cast<std::uint64_t>(device()) << 32) ^ device();
  std::ostringstream suffix;
  suffix << ".symenc-tmp-" << std::hex << std::setw(16) << std::setfill('0') << random;
  fs::path temp = dest;
  temp += suffix.str();
  return temp;
}

}

Bytes read_binary(std::istream& in) {
  Bytes data;
  std::vector<char> chunk(64 * 1024);
  do {
    in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
    const std::size_t got = static_cast<std::size_t>(in.gcount());
    if (got > 0) {
      const std::size_t old_size = data.size();
      data.resize(old_size + got);
      std::memcpy(data.data() + old_size, chunk.data(), got);
    }
  } while (in);
  if (in.bad()) throw IoError("read error");
  return data;
}

Bytes read_binary(const fs::path& file) {
  std::error_code ec;
  if (fs::is_directory(file, ec)) {
    throw IoError("cannot read '" + file.string() + "': it is a directory");
  }
  std::ifstream in(file, std::ios::binary);
  if (!in) throw IoError("cannot open '" + file.string() + "' for reading");
  return read_binary(in);
}

void write_binary_atomic(const fs::path& dest, std::string_view data) {
  const fs::path temp = temp_path_for(dest);
  {
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) throw IoError("cannot create a temporary file next to '" + dest.string() + "'");
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out.flush();
    out.close();
    if (out.fail()) {
      remove_quietly(temp);
      throw IoError("write to '" + temp.string() + "' failed");
    }
  }
  std::error_code ec;
  fs::rename(temp, dest, ec);
  if (ec) {
    remove_quietly(temp);
    throw IoError("cannot replace '" + dest.string() + "': " + ec.message());
  }
}

void write_binary(std::ostream& out, std::string_view data) {
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
  out.flush();
  if (!out) throw IoError("write to output stream failed");
}

}

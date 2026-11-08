#pragma once

#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace symtest {

class TempDir {
 public:
  TempDir() {
    std::random_device device;
    const std::uint64_t id = (static_cast<std::uint64_t>(device()) << 32) ^ device();
    path_ = std::filesystem::temp_directory_path() / ("symenc-test-" + std::to_string(id));
    std::filesystem::create_directories(path_);
  }
  ~TempDir() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }
  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  const std::filesystem::path& path() const { return path_; }
  std::filesystem::path file(const std::string& name) const { return path_ / name; }

 private:
  std::filesystem::path path_;
};

}

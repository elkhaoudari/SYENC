#include <exception>
#include <iostream>

#include "test_framework.hpp"

namespace symtest {

std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

namespace {
std::size_t g_failures_in_current_test = 0;
}

void report_failure(const char* file, int line, const std::string& message) {
  ++g_failures_in_current_test;
  std::cerr << file << ':' << line << ": " << message << '\n';
}

}

int main() {
  std::size_t failed_tests = 0;
  for (const auto& test : symtest::registry()) {
    symtest::g_failures_in_current_test = 0;
    std::cout << "[ RUN  ] " << test.name << '\n';
    try {
      test.fn();
    } catch (const std::exception& e) {
      symtest::report_failure(__FILE__, __LINE__, std::string("unexpected exception: ") + e.what());
    } catch (...) {
      symtest::report_failure(__FILE__, __LINE__, "unexpected non-standard exception");
    }
    if (symtest::g_failures_in_current_test == 0) {
      std::cout << "[  OK  ] " << test.name << '\n';
    } else {
      ++failed_tests;
      std::cout << "[FAILED] " << test.name << '\n';
    }
  }
  std::cout << symtest::registry().size() - failed_tests << '/' << symtest::registry().size()
            << " tests passed\n";
  return failed_tests == 0 ? 0 : 1;
}

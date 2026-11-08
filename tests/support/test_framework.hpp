#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace symtest {

struct TestCase {
  const char* name;
  void (*fn)();
};

std::vector<TestCase>& registry();
void report_failure(const char* file, int line, const std::string& message);

struct Registrar {
  Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

template <class T>
std::string show(const T& value) {
  std::ostringstream out;
  if constexpr (std::is_integral_v<T> && sizeof(T) == 1 && !std::is_same_v<T, bool>) {
    out << static_cast<int>(value);
  } else {
    out << value;
  }
  return out.str();
}

template <class A, class B>
void check_eq(const char* file, int line, const char* a_text, const char* b_text,
              const A& a, const B& b) {
  if (!(a == b)) {
    report_failure(file, line,
                   std::string("CHECK_EQ failed: ") + a_text + " == " + b_text +
                       "\n    left:  " + show(a) + "\n    right: " + show(b));
  }
}

}

#define SYMENC_TEST(name)                                                  \
  static void name();                                                      \
  static const ::symtest::Registrar registrar_##name{#name, &name};        \
  static void name()

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) ::symtest::report_failure(__FILE__, __LINE__,             \
                                           "CHECK failed: " #cond);        \
  } while (false)

#define CHECK_EQ(a, b) ::symtest::check_eq(__FILE__, __LINE__, #a, #b, (a), (b))

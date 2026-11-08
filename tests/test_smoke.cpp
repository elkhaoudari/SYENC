#include <string_view>

#include "symenc/version.hpp"
#include "test_framework.hpp"

SYMENC_TEST(spec_version_is_the_frozen_one) {
  CHECK_EQ(symenc::spec_version(), std::string_view("0.1.1"));
}

SYMENC_TEST(harness_check_passes_on_true_conditions) {
  CHECK(1 + 1 == 2);
  CHECK_EQ(3 * 5, 15);
  CHECK_EQ(std::string_view("JIG"), std::string_view("JIG"));
}

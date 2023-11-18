// External includes
#include <gtest/gtest.h>

// Local includes
#include "version.cpp"

TEST(version_test, runtime_version_matches_compiletime_version) {
    ASSERT_EQ(xor_crypt::get_runtime_version(), xor_crypt::compiletime_version);
}

#include <gtest/gtest.h>

TEST(Placeholder, Compiles) { EXPECT_TRUE(true); }

/** @brief Test entry point. Runs all GTest cases for the state machine. */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

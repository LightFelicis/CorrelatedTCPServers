#include <gtest/gtest.h>
#include "../connectionHandler.h"

TEST(correlated_files_parsing, simple) {
    auto c = CorrelatedFile("plik\t127.0.0.1\t8080");
    ASSERT_EQ(c.port, "8080");
    ASSERT_EQ(c.ipAddress, "127.0.0.1");
    ASSERT_EQ(c.resource, "plik");
}

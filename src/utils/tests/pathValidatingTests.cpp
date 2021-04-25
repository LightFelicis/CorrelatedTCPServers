#include <gtest/gtest.h>
#include "../pathUtils.h"

TEST(malicious, detected) {
    ASSERT_FALSE(validatePath("/tmp", "/../muahahah"));
    ASSERT_FALSE(validatePath("/tmp", "../muahahah"));
    ASSERT_FALSE(validatePath("/tmp", "/bump/../../muahahah"));
}

TEST(pathOK, ok) {
    ASSERT_TRUE(validatePath("/tmp", "/plik"));
    ASSERT_TRUE(validatePath("/tmp", "/katalog/../plik"));
}

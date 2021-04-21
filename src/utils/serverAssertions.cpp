#include <iostream>
#include <cstring>

#include "serverAssertions.h"

void exit_on_fail(bool condition, const std::string &error_message) {
    if (!condition) {
        std::cerr << error_message << std::endl;
    }
    exit(EXIT_FAILURE);
}


void exit_on_fail_with_errno(bool condition, const std::string &error_message) {
    if (!condition) {
        std::cerr << error_message << " errno: " << errno << " strerror: " << strerror(errno) << std::endl;
    }
    exit(EXIT_FAILURE);
}

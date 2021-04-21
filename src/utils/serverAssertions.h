#ifndef SIK_SERVERASSERTIONS_H
#define SIK_SERVERASSERTIONS_H
#include <iostream>

void exit_on_fail(bool condition, std::string error_message) {
    if (!condition) {
        std::cerr << error_message << std::endl;
    }
    exit(EXIT_FAILURE);
}

#endif //SIK_SERVERASSERTIONS_H

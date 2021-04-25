#ifndef ZALICZENIOWE1_SERVERASSERTIONS_H
#define ZALICZENIOWE1_SERVERASSERTIONS_H

void exit_on_fail(bool condition, const std::string &error_message);

void exit_on_fail_with_errno(bool condition, const std::string &error_message);

#endif //ZALICZENIOWE1_SERVERASSERTIONS_H

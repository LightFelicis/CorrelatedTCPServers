project(SIK)
cmake_minimum_required(VERSION 2.8.11)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

set(files
        src/utils/httpParsers.h
        src/server/connectionHandler.h
        src/utils/serverAssertions.h
        src/utils/serverAssertions.cpp
        src/utils/pathUtils.h
        src/utils/pathUtils.cpp
        )

# Locate GTest
find_package(GTest REQUIRED)

# Link runTests with what we want to test and the GTest library
add_executable(runTests src/utils/tests/parsingTests.cpp src/server/tests/parsingTests.cpp src/utils/tests/pathValidatingTests.cpp ${files})
add_executable(serwer src/server/server.cpp ${files})

target_link_libraries(runTests GTest::Main)
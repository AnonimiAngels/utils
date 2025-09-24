
CMAKE_MINIMUM_REQUIRED(VERSION 3.30...3.31)
PROJECT(tests LANGUAGES CXX)

add_subdirectory(test_common)
add_subdirectory(tests_cxx11)
add_subdirectory(tests_cxx14)
add_subdirectory(tests_cxx17)
add_subdirectory(tests_cxx20)
add_subdirectory(tests_cxx23)
add_subdirectory(tests_cxx26)
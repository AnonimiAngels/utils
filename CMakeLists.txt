cmake_minimum_required(VERSION 3.10...3.30)
project(utils VERSION 0.1.0 LANGUAGES CXX C)

SET(PROJECT_ALIAS ${PROJECT_NAME}::${PROJECT_NAME})

OPTION(BUILD_TESTS "Build the tests" OFF)
if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

FILE(GLOB_RECURSE SOURCES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*")
FILE(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h*")

SET(ALL_SOURCES ${SOURCES} ${HEADERS})
SET(LIBRARIES Threads::Threads fmt::fmt spdlog::spdlog)

if (CMAKE_CXX_STANDARD LESS 17)
	list(APPEND LIBRARIES ghc_filesystem)
	find_library(GHC_FILESYSTEM_LIB ghc_filesystem)
endif()

find_package(Threads REQUIRED)
find_library(FMT_LIB fmt)
find_library(SPDLOG_LIB spdlog)

add_library(${PROJECT_NAME} ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/inc"
	"${CMAKE_CURRENT_SOURCE_DIR}/inc/std"
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBRARIES})

add_library(${PROJECT_ALIAS} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
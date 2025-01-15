cmake_minimum_required(VERSION 3.29)

include(./scripts/CPM.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

set(LOGS_ENABLED OFF)
set(USE_CCACHE ON)
set(CCACHE_OPTIONS "CCACHE_CPP2=true;CCACHE_SLOPPINESS=clang_index_store")

CPMAddPackage(
	NAME Ccache.cmake
	GITHUB_REPOSITORY TheLartians/Ccache.cmake
	VERSION 1.2.5
)

set(PROJECT_NAME utils)
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
project(${PROJECT_NAME})

# Collect sources and headers
set(SRC_DIRS ${PROJECT_DIR}/src)
set(INC_DIRS ${PROJECT_DIR}/inc ${PROJECT_DIR}/inc/common)

foreach(dir ${SRC_DIRS})
	file(GLOB_RECURSE PROJECT_SOURCES ${dir}/*.c*)
endforeach()

foreach(dir ${INC_DIRS})
	file(GLOB_RECURSE PROJECT_HEADERS ${dir}/*.h*)
endforeach()

# Set the output directory for the library
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})

set(EXPORT_INC_DIRS ${PROJECT_DIR}/inc/common ${PROJECT_DIR}/inc)

if (LOGS_ENABLED)
message(STATUS "Project ${PROJECT_NAME} INC_DIRS: ${INC_DIRS}")
message(STATUS "Project ${PROJECT_NAME} EXPORT_INC_DIRS: ${EXPORT_INC_DIRS}")
endif()

# list(REMOVE_ITEM INC_DIRS ${EXPORT_INC_DIRS})

# Add library
add_library(${PROJECT_NAME} INTERFACE ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
target_include_directories(${PROJECT_NAME} INTERFACE ${EXPORT_INC_DIRS})

if (LOGS_ENABLED)
	message(STATUS "Project sources: ${PROJECT_SOURCES}")
	message(STATUS "Project include directories: ${INC_DIRS}")
	message(STATUS "Project libraries: ${LIBRARIES}")
endif()
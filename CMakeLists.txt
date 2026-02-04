cmake_minimum_required(VERSION 3.14...3.31)
project(grid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(SOURCES
	src/main.cc
	src/image.cc
)

add_executable(grid ${SOURCES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(grid PRIVATE
		-flto -ffast-math -ffast-math
		-O3
		-Wall -Wextra -Werror=return-type -fno-rtti
	)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(grid PRIVATE
		/O2
		/W4
		/permissive-
	)
endif()

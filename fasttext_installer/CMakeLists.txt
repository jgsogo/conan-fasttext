
cmake_minimum_required(VERSION 2.8)
project(fasttext CXX)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Write the CMake from scratch
add_executable(fasttext main.cc)
target_link_libraries(fasttext CONAN_PKG::fasttext)
target_include_directories(fasttext PRIVATE "${CONAN_INCLUDE_DIRS_FASTTEXT}/fasttext")
set_target_properties(fasttext PROPERTIES CXX_STANDARD 11)

install (TARGETS fasttext RUNTIME DESTINATION bin)

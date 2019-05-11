
cmake_minimum_required(VERSION 2.8)
project(fasttext CXX)

include(conanbuildinfo.cmake)
conan_basic_setup()

# Write the CMake from scratch

set(HEADER_FILES
    fasttext/src/args.h
    fasttext/src/dictionary.h
    fasttext/src/fasttext.h
    fasttext/src/matrix.h
    fasttext/src/meter.h
    fasttext/src/model.h
    fasttext/src/productquantizer.h
    fasttext/src/qmatrix.h
    fasttext/src/real.h
    fasttext/src/utils.h
    fasttext/src/vector.h)

set(SOURCE_FILES
    fasttext/src/args.cc
    fasttext/src/dictionary.cc
    fasttext/src/fasttext.cc
    fasttext/src/main.cc
    fasttext/src/matrix.cc
    fasttext/src/meter.cc
    fasttext/src/model.cc
    fasttext/src/productquantizer.cc
    fasttext/src/qmatrix.cc
    fasttext/src/utils.cc
    fasttext/src/vector.cc)

add_library(fasttext ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(fasttext PROPERTIES 
            PUBLIC_HEADER "${HEADER_FILES}"
            CXX_STANDARD 11)
install (TARGETS fasttext
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/fasttext
    )

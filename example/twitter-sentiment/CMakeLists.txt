cmake_minimum_required(VERSION 2.8)
project(twitter-sentiment)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(rxcpp REQUIRED)
find_package(fmt REQUIRED)
find_package(oauth REQUIRED)
find_package(jsonformoderncpp REQUIRED)
find_package(range-v3 REQUIRED)
find_package(libpqxx REQUIRED)

add_subdirectory(src)

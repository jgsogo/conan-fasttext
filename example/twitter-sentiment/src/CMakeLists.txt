

add_executable(app main.cpp twitter.cpp twitter.h rxcurl.h)
target_link_libraries(app rxcpp::rxcpp fmt::fmt oauth::oauth jsonformoderncpp::jsonformoderncpp range-v3::range-v3)

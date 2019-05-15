

add_executable(app main.cpp twitter.cpp twitter.h tweet.h tweet.cpp rxcurl.h rxcurl.cpp utils.h utils.cpp)
target_link_libraries(app rxcpp::rxcpp fmt::fmt oauth::oauth jsonformoderncpp::jsonformoderncpp range-v3::range-v3)

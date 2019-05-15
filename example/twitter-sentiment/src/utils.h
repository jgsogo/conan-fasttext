
#pragma once

#include <string>
#include <vector>


namespace utils {

    enum class Split {
        KeepDelimiter,
        RemoveDelimiter,
        OnlyDelimiter
    };

    std::vector<std::string> split(std::string s, std::string d, Split m = Split::KeepDelimiter);

    std::string tolower(std::string s);

    std::vector<std::string> splitwords(const std::string& text);
}

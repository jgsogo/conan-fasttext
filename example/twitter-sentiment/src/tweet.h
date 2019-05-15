
#pragma once

#include <string>
#include <nlohmann/json.hpp>


namespace twitter {

    std::string tweettext(const nlohmann::json& tweet);

    struct Tweet
    {
        Tweet();
        explicit Tweet(const nlohmann::json& tweet);

        struct shared
        {
            shared();
            explicit shared(const nlohmann::json& t);
            nlohmann::json tweet;
            std::vector<std::string> words;
        };
        std::shared_ptr<const shared> data = std::make_shared<shared>();
    };

}
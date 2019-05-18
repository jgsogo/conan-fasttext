
#pragma once

#include <string>
#include <nlohmann/json.hpp>


namespace twitter {

    std::string tweettext(const nlohmann::json& tweet);

    struct Tweet
    {
        Tweet();
        explicit Tweet(const nlohmann::json& tweet);

        std::string id_str() const;
        std::string user_id() const;
        std::string lang() const;
        std::string text() const;
        time_t timestamp() const;
        std::vector<std::string> hashtags() const;

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

#include "tweet.h"

#include "utils.h"

namespace twitter {

    std::string tweettext(const nlohmann::json& tweet) {
        if (!!tweet.count("extended_tweet")) {
            auto ex = tweet["extended_tweet"];
            if (!!ex.count("full_text") && ex["full_text"].is_string()) {
                return ex["full_text"];
            }
        }
        if (!!tweet.count("text") && tweet["text"].is_string()) {
            return tweet["text"];
        }
        return {};
    }


    Tweet::Tweet() {}
    Tweet::Tweet(const nlohmann::json& tweet)
            : data(std::make_shared<shared>(shared{tweet}))
    {}

    Tweet::shared::shared() {}
    Tweet::shared::shared(const nlohmann::json& t)
            : tweet(t)
            , words(utils::splitwords(tweettext(tweet)))
    {}

}
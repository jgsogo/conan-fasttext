
#include "tweet.h"

#include <sstream>
#include <iomanip>
#include <iostream>
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


    std::string Tweet::id_str() const {
        return this->data->tweet["id_str"];
    }

    std::string Tweet::user_id() const {
        return this->data->tweet["user"]["id_str"];
    }

    std::string Tweet::lang() const {
        if (!!this->data->tweet.count("lang")) {
            return this->data->tweet["lang"];
        }
        return {};
    }

    std::string Tweet::text() const {
        return tweettext(this->data->tweet);
    }

    time_t Tweet::timestamp() const {
        return std::stoll(this->data->tweet["timestamp_ms"].get<std::string>());
        /*
        if (!!this->data->tweet.count("created_at") && this->data->tweet["created_at"].is_string()) {
            //return this->data->tweet["created_at"];

            // Parse datetime: Sat May 18 10:41:16 +0000 2019
            std::istringstream date_stream{std::string{this->data->tweet["created_at"]}};
            std::tm time_date{};
            date_stream >> std::get_time(&time_date, "%a %b %d %H:%M:%S +0000 %Y");
            if(!date_stream.fail()) return timegm(&time_date);;
        }
        return {};
         */
    }

    std::vector<std::string> Tweet::hashtags() const {
        std::vector<std::string> ret;
        for(auto& it: this->data->tweet["entities"]["hashtags"]) {
            ret.push_back(it["text"]);
        }
        return ret;
    }

}
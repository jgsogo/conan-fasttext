
#pragma once

#include <string>

#include <rxcpp/rx.hpp>

#include "rxcurl.h"
#include "tweet.h"


namespace twitter {

    rxcpp::observable<std::string> twitterrequest(rxcpp::observe_on_one_worker tweetthread, rxcurl::rxcurl factory, std::string URL, std::string method,
                             std::string CONS_KEY, std::string CONS_SEC, std::string ATOK_KEY, std::string ATOK_SEC);


    struct parseerror {
        std::exception_ptr ep;
    };
    struct parsedtweets {
        rxcpp::observable<Tweet> tweets;
        rxcpp::observable<parseerror> errors;
    };

    auto parsetweets(rxcpp::observe_on_one_worker worker, rxcpp::observe_on_one_worker tweetthread) -> std::function<rxcpp::observable<parsedtweets>(rxcpp::observable<std::string>)>;
}
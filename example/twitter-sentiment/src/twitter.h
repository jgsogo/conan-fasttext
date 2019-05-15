
#pragma once

#include <string>
#include <regex>

#include <oauth.h>
#include <rxcpp/rx.hpp>
#include <nlohmann/json.hpp>
#include <range/v3/all.hpp>

#include "rxcurl.h"
#include "utils.h"


namespace twitter {

    std::function<rxcpp::observable<std::string>(rxcpp::observable<long>)> stringandignore();

    rxcpp::observable<std::string> twitterrequest(rxcpp::observe_on_one_worker tweetthread, rxcurl::rxcurl factory, std::string URL, std::string method,
                             std::string CONS_KEY, std::string CONS_SEC, std::string ATOK_KEY, std::string ATOK_SEC);


    inline std::string tweettext(const nlohmann::json& tweet) {
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



    struct Tweet
    {
        Tweet() {}
        explicit Tweet(const nlohmann::json& tweet)
                : data(std::make_shared<shared>(shared{tweet}))
        {}
        struct shared
        {
            shared() {}
            explicit shared(const nlohmann::json& t)
                    : tweet(t)
                    , words(utils::splitwords(tweettext(tweet)))
            {}
            nlohmann::json tweet;
            std::vector<std::string> words;
        };
        std::shared_ptr<const shared> data = std::make_shared<shared>();
    };

    struct parseerror {
        std::exception_ptr ep;
    };
    struct parsedtweets {
        rxcpp::observable<Tweet> tweets;
        rxcpp::observable<parseerror> errors;
    };

    auto isEndOfTweet = [](const std::string& s){
        if (s.size() < 2) return false;
        auto it0 = s.begin() + (s.size() - 2);
        auto it1 = s.begin() + (s.size() - 1);
        return *it0 == '\r' && *it1 == '\n';
    };

    inline auto parsetweets(rxcpp::observe_on_one_worker worker, rxcpp::observe_on_one_worker tweetthread) -> std::function<rxcpp::observable<parsedtweets>(rxcpp::observable<std::string>)> {
        return [=](rxcpp::observable<std::string> chunks) -> rxcpp::observable<parsedtweets> {
            return rxcpp::rxs::create<parsedtweets>([=](rxcpp::subscriber<parsedtweets> out){
                // create strings split on \r
                auto strings = chunks |
                               rxcpp::operators::concat_map([](const std::string& s){
                                   auto splits = utils::split(s, "\r\n");
                                   return rxcpp::sources::iterate(move(splits));
                               }) |
                               rxcpp::operators::filter([](const std::string& s){
                                   return !s.empty();
                               }) |
                               rxcpp::operators::publish() |
                               rxcpp::operators::ref_count();

                // filter to last string in each line
                auto closes = strings |
                              rxcpp::operators::filter(isEndOfTweet) |
                              rxcpp::rxo::map([](const std::string&){return 0;});

                // group strings by line
                auto linewindows = strings |
                                   window_toggle(closes | rxcpp::operators::start_with(0), [=](int){return closes;});

                // reduce the strings for a line into one string
                auto lines = linewindows |
                             rxcpp::operators::flat_map([](const rxcpp::observable<std::string>& w) {
                                 return w | rxcpp::operators::start_with<std::string>("") | rxcpp::operators::sum();
                             });

                int count = 0;
                rxcpp::rxsub::subject<parseerror> errorconduit;
                rxcpp::observable<Tweet> tweets = lines |
                                           rxcpp::operators::filter([](const std::string& s){
                                               return s.size() > 2 && s.find_first_not_of("\r\n") != std::string::npos;
                                           }) |
                                           rxcpp::operators::group_by([count](const std::string&) mutable -> int {
                                               return ++count % std::thread::hardware_concurrency();}) |
                                           rxcpp::rxo::map([=](rxcpp::observable<std::string> shard) {
                                               return shard |
                                                      rxcpp::operators::observe_on(worker) |
                                                      rxcpp::rxo::map([=](const std::string& line) -> rxcpp::observable<Tweet> {
                                                          try {
                                                              auto tweet = nlohmann::json::parse(line);
                                                              return rxcpp::rxs::from(Tweet(tweet));
                                                          } catch (...) {
                                                              errorconduit.get_subscriber().on_next(parseerror{std::current_exception()});
                                                          }
                                                          return rxcpp::rxs::empty<Tweet>();
                                                      }) |
                                                      rxcpp::operators::merge() |
                                                      rxcpp::operators::as_dynamic();
                                           }) |
                                           rxcpp::operators::merge(tweetthread) |
                                           rxcpp::operators::tap([](const Tweet&){},[=](){
                                               errorconduit.get_subscriber().on_completed();
                                           }) |
                                           rxcpp::operators::finally([=](){
                                               errorconduit.get_subscriber().unsubscribe();
                                           });

                out.on_next(parsedtweets{tweets, errorconduit.get_observable()});
                out.on_completed();

                return out.get_subscription();
            });
        };
    }
}
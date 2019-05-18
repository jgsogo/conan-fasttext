
#include <string>
#include <iostream>
#include <fmt/format.h>
#include "twitter.h"
#include "db/database.h"

#include <range/v3/all.hpp>


std::string humanize(time_t tm) {
    char buf[sizeof "2011-10-08 07:07"];
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", gmtime(&tm));
    return std::string(buf);
}

std::string get_env(const std::string& env_var) {
    const char* value = std::getenv(env_var.c_str());
    if (!value) {
        throw std::runtime_error(fmt::format("Provide env variable '{}'", env_var));
    }
    return value;
}


int main() {
    // Inputs related to Twitter API
    const std::string tw_consumer_key = "hOPTbPoOi3i38WFW2mWSoMtnb";
    const std::string tw_access_token = "332912007-O2ZZQqICUcRNaImFuuVzyQCstVGo6giphaaJ5Pvu";
    const std::string tw_consumer_secret = get_env("TW_CONSUMER_SECRET");
    const std::string tw_access_token_secret = get_env("TW_ACCESS_TOKEN_SECRET");

    // Create database if not exists
    db::Database::instance().tweets().create();

    auto tweetthread = rxcpp::observe_on_new_thread();
    auto poolthread = rxcpp::observe_on_event_loop();
    auto factory = rxcurl::create_rxcurl();
    rxcpp::composite_subscription lifetime;

    const std::string tw_url_sample = "https://stream.twitter.com/1.1/statuses/sample.json";
    const std::string tw_url_filter = "https://stream.twitter.com/1.1/statuses/filter.json?track=eurovision&language=en";
    bool isFilter = true;
    std::string method = isFilter ? "POST" : "GET";
    std::string url = tw_url_filter;

    rxcpp::observable<std::string> chunks;
    chunks = twitter::twitterrequest(tweetthread, factory, url, method, tw_consumer_key, tw_consumer_secret, tw_access_token, tw_access_token_secret) |
             // handle invalid requests by waiting for a trigger to try again
             rxcpp::operators::on_error_resume_next([](std::exception_ptr ep){
                 std::cerr << rxcpp::rxu::what(ep) << std::endl;
                 return rxcpp::rxs::never<std::string>();
             });

    auto tweets = chunks |
                  twitter::parsetweets(poolthread, tweetthread) |
                  rxcpp::rxo::map([](twitter::parsedtweets p){
                      p.errors |
                      rxcpp::operators::tap([](twitter::parseerror e){
                          std::cerr << rxcpp::rxu::what(e.ep) << std::endl;
                      }) |
                      rxcpp::operators::subscribe<twitter::parseerror>();
                      return p.tweets;
                  }) |
                  rxcpp::operators::merge(tweetthread);

    //tweets |
    //    rxcpp::rxo::map([](twitter::Tweet& t) {
    //        return t.text();
    //        /*return (t.data->words | ranges::view::join(',') | ranges::to_<std::string>());*/
    //    }) |
    //    rxcpp::operators::subscribe<std::string>(rxcpp::util::println(std::cout));


    auto batch_tweets = tweets |
                        twitter::onlytweets() |
                        rxcpp::rxo::buffer_with_time(std::chrono::milliseconds(2000), poolthread) |
                        rxcpp::rxo::filter([](const std::vector<twitter::Tweet>& tws){ return !tws.empty(); }) |
                        rxcpp::rxo::publish() |
                        rxcpp::rxo::ref_count() |
                        rxcpp::rxo::as_dynamic();

    // store tweets in the database (once every 2 seconds)
    batch_tweets |
            rxcpp::operators::subscribe<std::vector<twitter::Tweet>>([](std::vector<twitter::Tweet> tws) {
                std::vector<db::Tweet> db_tweets; db_tweets.reserve(tws.size());
                for (auto& tw: tws) {
                    const std::vector<std::string>& hashtags{tw.hashtags()};
                    std::string hashtags_as_str{(hashtags | ranges::view::join(',') | ranges::to_<std::string>())};
                    db_tweets.emplace_back(std::move(tw.timestamp()), std::move(hashtags_as_str), std::move(tw.text()));
                }
                std::cout << "About to save '" << tws.size() << "' tweets\n";
                db::Database::instance().tweets().insert(db_tweets);
            });


    // Count total number of tweets (interval 5 seconds)
    batch_tweets |
        rxcpp::rxo::map([](const std::vector<twitter::Tweet>& tws) { return tws.size(); }) |
        rxcpp::rxo::scan(0, [](int seed, int last_batch) {
            return seed+last_batch;
        }) |
        rxcpp::operators::subscribe<int>(rxcpp::util::println(std::cout));
    return 0;
}

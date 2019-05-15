
#include <string>
#include <iostream>
#include <fmt/format.h>
#include "twitter.h"



std::string get_env(const std::string& env_var) {
    const char* value = std::getenv(env_var.c_str());
    if (!value) {
        throw std::runtime_error(fmt::format("Provide env variable '{}'", env_var));
    }
    return value;
}


int main() {
    // Inputs related to Twitter API
    const std::string tw_url = "https://stream.twitter.com/1.1/statuses/sample.json";
    const std::string tw_consumer_key = "hOPTbPoOi3i38WFW2mWSoMtnb";
    const std::string tw_access_token = "332912007-O2ZZQqICUcRNaImFuuVzyQCstVGo6giphaaJ5Pvu";
    const std::string tw_consumer_secret = get_env("TW_CONSUMER_SECRET");
    const std::string tw_access_token_secret = get_env("TW_ACCESS_TOKEN_SECRET");


    auto tweetthread = rxcpp::observe_on_new_thread();
    auto poolthread = rxcpp::observe_on_event_loop();
    auto factory = rxcurl::create_rxcurl();
    rxcpp::composite_subscription lifetime;

    bool isFilter = false;
    std::string method = isFilter ? "POST" : "GET";
    std::string url = tw_url;

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



    // print result
    tweets |
        rxcpp::rxo::map([](twitter::Tweet& t) {
            /* twitter::tweettext(t.data->tweet);*/
            return (t.data->words | ranges::view::join(',') | ranges::to_<std::string>());
        }) |
        rxcpp::operators::subscribe<std::string>(rxcpp::util::println(std::cout));

    return 0;
}

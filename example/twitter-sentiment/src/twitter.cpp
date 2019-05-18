
#include "twitter.h"

#include <oauth.h>
#include "utils.h"


namespace twitter {

    std::function<rxcpp::observable<std::string>(rxcpp::observable<long>)> stringandignore() {
        return [](rxcpp::observable<long> s){
            return s.map([](long){return std::string{};}).ignore_elements();
        };
    }

    auto twitter_stream_reconnection(rxcpp::observe_on_one_worker tweetthread) {
        return [=](rxcpp::observable<std::string> chunks){
            return chunks |
                   // https://dev.twitter.com/streaming/overview/connecting
                   rxcpp::operators::timeout(std::chrono::seconds(90), tweetthread) |
                   rxcpp::operators::on_error_resume_next([=](std::exception_ptr ep) -> rxcpp::observable<std::string> {
                       try {rethrow_exception(ep);}
                       catch (const rxcurl::http_exception& ex) {
                           std::cerr << ex.what() << std::endl;
                           switch(rxcurl::errorclassfrom(ex)) {
                               case rxcurl::errorcodeclass::TcpRetry:
                                   std::cerr << "reconnecting after TCP error" << std::endl;
                                   return rxcpp::observable<>::empty<std::string>();
                               case rxcurl::errorcodeclass::ErrorRetry:
                                   std::cerr << "error code (" << ex.code() << ") - ";
                               case rxcurl::errorcodeclass::StatusRetry:
                                   std::cerr << "http status (" << ex.httpStatus() << ") - waiting to retry.." << std::endl;
                                   return rxcpp::observable<>::timer(std::chrono::seconds(5), tweetthread) | stringandignore();
                               case rxcurl::errorcodeclass::RateLimited:
                                   std::cerr << "rate limited - waiting to retry.." << std::endl;
                                   return rxcpp::observable<>::timer(std::chrono::minutes(1), tweetthread) | stringandignore();
                               case rxcurl::errorcodeclass::Invalid:
                                   std::cerr << "invalid request - propagate" << std::endl;
                               default:
                                   std::cerr << "unrecognized error - propagate" << std::endl;
                           };
                       }
                       catch (const rxcpp::timeout_error& ex) {
                           std::cerr << "reconnecting after timeout" << std::endl;
                           return rxcpp::observable<>::empty<std::string>();
                       }
                       catch (const std::exception& ex) {
                           std::cerr << "unknown exception - terminate" << std::endl;
                           std::cerr << ex.what() << std::endl;
                           std::terminate();
                       }
                       catch (...) {
                           std::cerr << "unknown exception - not derived from std::exception - terminate" << std::endl;
                           std::terminate();
                       }
                       return rxcpp::observable<>::error<std::string>(ep, tweetthread);
                   }) |
                   rxcpp::operators::repeat();
        };
    }

    rxcpp::observable<std::string> twitterrequest(rxcpp::observe_on_one_worker tweetthread, rxcurl::rxcurl factory, std::string URL, std::string method,
                        std::string CONS_KEY, std::string CONS_SEC, std::string ATOK_KEY, std::string ATOK_SEC) {

        //return rxcpp::observable<>::defer([=]() {

        std::string url;
        {
            char *signedurl = nullptr;
            RXCPP_UNWIND_AUTO([&]() {
                if (!!signedurl) {
                    free(signedurl);
                }
            });
            signedurl = oauth_sign_url2(
                    URL.c_str(), NULL, OA_HMAC, method.c_str(),
                    CONS_KEY.c_str(), CONS_SEC.c_str(), ATOK_KEY.c_str(), ATOK_SEC.c_str()
            );
            url = signedurl;
        }

        std::cerr << "start twitter stream request" << std::endl;

        return factory.create(rxcurl::http_request{url, method, {}, {}}) |
               rxcpp::rxo::map([](rxcurl::http_response r) {
                   return r.body.chunks;
               }) |
               rxcpp::operators::finally([]() { std::cerr << "end twitter stream request" << std::endl; }) |
               rxcpp::operators::merge(tweetthread)
               /*})*/ |
               twitter_stream_reconnection(tweetthread);
    }


    auto isEndOfTweet = [](const std::string& s){
        if (s.size() < 2) return false;
        auto it0 = s.begin() + (s.size() - 2);
        auto it1 = s.begin() + (s.size() - 1);
        return *it0 == '\r' && *it1 == '\n';
    };

    auto parsetweets(rxcpp::observe_on_one_worker worker, rxcpp::observe_on_one_worker tweetthread) -> std::function<rxcpp::observable<parsedtweets>(rxcpp::observable<std::string>)> {
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

    auto onlytweets() -> std::function<rxcpp::observable<Tweet>(rxcpp::observable<Tweet>)> {
        return [](rxcpp::observable<Tweet> s){
            return s | rxcpp::rxo::filter([](const Tweet& tw){
                auto& tweet = tw.data->tweet;
                return !!tweet.count("timestamp_ms");
            });
        };
    }
}

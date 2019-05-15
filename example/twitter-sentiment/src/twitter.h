
#pragma once

#include <string>
#include <regex>

#include <oauth.h>
#include <rxcpp/rx.hpp>
#include <nlohmann/json.hpp>
#include <range/v3/all.hpp>

#include "rxcurl.h"


namespace twitter {

    enum class errorcodeclass {
        Invalid,
        TcpRetry,
        ErrorRetry,
        StatusRetry,
        RateLimited
    };

    inline errorcodeclass errorclassfrom(const rxcurl::http_exception& ex) {
        switch(ex.code()) {
            case CURLE_COULDNT_RESOLVE_HOST:
            case CURLE_COULDNT_CONNECT:
            case CURLE_OPERATION_TIMEDOUT:
            case CURLE_BAD_CONTENT_ENCODING:
            case CURLE_REMOTE_FILE_NOT_FOUND:
                return errorcodeclass::ErrorRetry;
            case CURLE_GOT_NOTHING:
            case CURLE_PARTIAL_FILE:
            case CURLE_SEND_ERROR:
            case CURLE_RECV_ERROR:
                return errorcodeclass::TcpRetry;
            default:
                if (ex.code() == CURLE_HTTP_RETURNED_ERROR || ex.httpStatus() > 200) {
                    if (ex.httpStatus() == 420) {
                        return errorcodeclass::RateLimited;
                    } else if (ex.httpStatus() == 404 ||
                               ex.httpStatus() == 406 ||
                               ex.httpStatus() == 413 ||
                               ex.httpStatus() == 416) {
                        return errorcodeclass::Invalid;
                    }
                }
        };
        return errorcodeclass::StatusRetry;
    }

    inline std::function<rxcpp::observable<std::string>(rxcpp::observable<long>)> stringandignore() {
        return [](rxcpp::observable<long> s){
            return s.map([](long){return std::string{};}).ignore_elements();
        };
    }

    auto twitter_stream_reconnection = [](rxcpp::observe_on_one_worker tweetthread){
        return [=](rxcpp::observable<std::string> chunks){
            return chunks |
                   // https://dev.twitter.com/streaming/overview/connecting
                   rxcpp::operators::timeout(std::chrono::seconds(90), tweetthread) |
                   rxcpp::operators::on_error_resume_next([=](std::exception_ptr ep) -> rxcpp::observable<std::string> {
                       try {rethrow_exception(ep);}
                       catch (const rxcurl::http_exception& ex) {
                           std::cerr << ex.what() << std::endl;
                           switch(errorclassfrom(ex)) {
                               case errorcodeclass::TcpRetry:
                                   std::cerr << "reconnecting after TCP error" << std::endl;
                                   return rxcpp::observable<>::empty<std::string>();
                               case errorcodeclass::ErrorRetry:
                                   std::cerr << "error code (" << ex.code() << ") - ";
                               case errorcodeclass::StatusRetry:
                                   std::cerr << "http status (" << ex.httpStatus() << ") - waiting to retry.." << std::endl;
                                   return rxcpp::observable<>::timer(std::chrono::seconds(5), tweetthread) | stringandignore();
                               case errorcodeclass::RateLimited:
                                   std::cerr << "rate limited - waiting to retry.." << std::endl;
                                   return rxcpp::observable<>::timer(std::chrono::minutes(1), tweetthread) | stringandignore();
                               case errorcodeclass::Invalid:
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
    };


    auto twitterrequest = [](rxcpp::observe_on_one_worker tweetthread, rxcurl::rxcurl factory, std::string URL, std::string method,
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
                       std::cerr << "Here we have a boody.chunks!!\n";
                       return r.body.chunks;
                   }) |
                   rxcpp::operators::finally([]() { std::cerr << "end twitter stream request" << std::endl; }) |
                   rxcpp::operators::merge(tweetthread)
        /*})*/ |
               twitter_stream_reconnection(tweetthread);
    };


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

    enum class Split {
        KeepDelimiter,
        RemoveDelimiter,
        OnlyDelimiter
    };

    auto split = [](std::string s, std::string d, Split m = Split::KeepDelimiter){
        std::regex delim(d);
        std::cregex_token_iterator cursor(&s[0], &s[0] + s.size(), delim, m == Split::KeepDelimiter ? std::initializer_list<int>({-1, 0}) : (m == Split::RemoveDelimiter ? std::initializer_list<int>({-1}) : std::initializer_list<int>({0})));
        std::cregex_token_iterator end;
        std::vector<std::string> splits(cursor, end);
        return splits;
    };

    inline std::string tolower(std::string s) {
        transform(s.begin(), s.end(), s.begin(), [=](char c){return std::tolower(c);});
        return s;
    }

    inline std::vector<std::string> splitwords(const std::string& text) {

        static const std::unordered_set<std::string> ignoredWords{
                // added
                "rt", "like", "just", "tomorrow", "new", "year", "month", "day", "today", "make", "let", "want", "did", "going", "good", "really", "know", "people", "got", "life", "need", "say", "doing", "great", "right", "time", "best", "happy", "stop", "think", "world", "watch", "gonna", "remember", "way",
                "better", "team", "check", "feel", "talk", "hurry", "look", "live", "home", "game", "run", "i'm", "you're", "person", "house", "real", "thing", "lol", "has", "things", "that's", "thats", "fine", "i've", "you've", "y'all", "didn't", "said", "come", "coming", "haven't", "won't", "can't", "don't",
                "shouldn't", "hasn't", "doesn't", "i'd", "it's", "i'll", "what's", "we're", "you'll", "let's'", "lets", "vs", "win", "says", "tell", "follow", "comes", "look", "looks", "post", "join", "add", "does", "went", "sure", "wait", "seen", "told", "yes", "video", "lot", "looks", "long",
                "e280a6", "\xe2\x80\xa6",
                // http://xpo6.com/list-of-english-stop-words/
                "a", "about", "above", "above", "across", "after", "afterwards", "again", "against", "all", "almost", "alone", "along", "already", "also","although","always","am","among", "amongst", "amoungst", "amount",  "an", "and", "another", "any","anyhow","anyone","anything","anyway", "anywhere", "are", "around", "as",  "at", "back","be","became", "because","become","becomes", "becoming", "been", "before", "beforehand", "behind", "being", "below", "beside", "besides", "between", "beyond", "bill", "both", "bottom","but", "by", "call", "can", "cannot", "cant", "co", "con", "could", "couldnt", "cry", "de", "describe", "detail", "do", "done", "down", "due", "during", "each", "eg", "eight", "either", "eleven","else", "elsewhere", "empty", "enough", "etc", "even", "ever", "every", "everyone", "everything", "everywhere", "except", "few", "fifteen", "fify", "fill", "find", "fire", "first", "five", "for", "former", "formerly", "forty", "found", "four", "from", "front", "full", "further", "get", "give", "go", "had", "has", "hasnt", "have", "he", "hence", "her", "here", "hereafter", "hereby", "herein", "hereupon", "hers", "herself", "him", "himself", "his", "how", "however", "hundred", "ie", "if", "in", "inc", "indeed", "interest", "into", "is", "it", "its", "itself", "keep", "last", "latter", "latterly", "least", "less", "ltd", "made", "many", "may", "me", "meanwhile", "might", "mill", "mine", "more", "moreover", "most", "mostly", "move", "much", "must", "my", "myself", "name", "namely", "neither", "never", "nevertheless", "next", "nine", "no", "nobody", "none", "noone", "nor", "not", "nothing", "now", "nowhere", "of", "off", "often", "on", "once", "one", "only", "onto", "or", "other", "others", "otherwise", "our", "ours", "ourselves", "out", "over", "own","part", "per", "perhaps", "please", "put", "rather", "re", "same", "see", "seem", "seemed", "seeming", "seems", "serious", "several", "she", "should", "show", "side", "since", "sincere", "six", "sixty", "so", "some", "somehow", "someone", "something", "sometime", "sometimes", "somewhere", "still", "such", "system", "take", "ten", "than", "that", "the", "their", "them", "themselves", "then", "thence", "there", "thereafter", "thereby", "therefore", "therein", "thereupon", "these", "they", "thickv", "thin", "third", "this", "those", "though", "three", "through", "throughout", "thru", "thus", "to", "together", "too", "top", "toward", "towards", "twelve", "twenty", "two", "un", "under", "until", "up", "upon", "us", "very", "via", "was", "we", "well", "were", "what", "whatever", "when", "whence", "whenever", "where", "whereafter", "whereas", "whereby", "wherein", "whereupon", "wherever", "whether", "which", "while", "whither", "who", "whoever", "whole", "whom", "whose", "why", "will", "with", "within", "without", "would", "yet", "you", "your", "yours", "yourself", "yourselves", "the"};

        static const std::string delimiters = R"(\s+)";
        auto words = split(text, delimiters, Split::RemoveDelimiter);

        // exclude entities, urls and some punct from this words list

        static const std::regex ignore(R"((\xe2\x80\xa6)|(&[\w]+;)|((http|ftp|https)://[\w-]+(.[\w-]+)+([\w.,@?^=%&:/~+#-]*[\w@?^=%&/~+#-])?))");
        static const std::regex expletives(R"(\x66\x75\x63\x6B|\x73\x68\x69\x74|\x64\x61\x6D\x6E)");

        for (auto& word: words) {
            while (!word.empty() && (word.front() == '.' || word.front() == '(' || word.front() == '\'' || word.front() == '\"')) word.erase(word.begin());
            while (!word.empty() && (word.back() == ':' || word.back() == ',' || word.back() == ')' || word.back() == '\'' || word.back() == '\"')) word.resize(word.size() - 1);
            if (!word.empty() && word.front() == '@') continue;
            word = regex_replace(tolower(word), ignore, "");
            if (!word.empty() && word.front() != '#') {
                while (!word.empty() && ispunct(word.front())) word.erase(word.begin());
                while (!word.empty() && ispunct(word.back())) word.resize(word.size() - 1);
            }
            word = regex_replace(word, expletives, "<expletive>");
        }

        words.erase(std::remove_if(words.begin(), words.end(), [=](const std::string& w){
            return !(w.size() > 2 && ignoredWords.find(w) == ignoredWords.end());
        }), words.end());

        words |=
                ranges::action::sort |
                ranges::action::unique;

        return words;
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
                    , words(splitwords(tweettext(tweet)))
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
                                   auto splits = split(s, "\r\n");
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
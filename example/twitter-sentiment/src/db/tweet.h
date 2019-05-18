
#pragma once

#include <tuple>
#include <pqxx/pqxx>

namespace db {

    typedef std::tuple<time_t, std::string, std::string, std::string> Tweet; // timestamp_ms, lang, hashtags, text

    class TweetManager {
    public:
        explicit TweetManager(pqxx::connection&);

        void create();
        void remove();

        std::vector<Tweet> all();
        void insert(time_t timestamp, const std::string&, const std::string& hashtags, const std::string& message);
        void insert(const std::vector<Tweet>& data);
        std::vector<Tweet> filter(time_t init, time_t end);

        //void update(const Tweet& tweet);
        //void remove(Tweet& tweet);
        //Tweet get(int id);

    protected:
        pqxx::connection& _connection;
    };

}
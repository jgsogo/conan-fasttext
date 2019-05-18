
#include "tweet.h"

#include <iomanip>
#include <sstream>
#include <iostream>
#include <fmt/format.h>

namespace db {

    namespace {
        const std::string table_name = "tweets";
        const std::vector<std::string> fields = {"timestamp_ms", "hashtags", "text"};

        pqxx::result run_query(pqxx::connection &connection, const std::string &query) {
            pqxx::work work(connection);
            try {
                auto r = work.exec(query);
                work.commit();
                return r;
            } catch (const std::exception &e) {
                work.abort();
                throw;
            }
        }
    }


    TweetManager::TweetManager(pqxx::connection &connection) : _connection(connection) {}

    void TweetManager::create() {
        auto result = run_query(_connection, fmt::format("SELECT to_regclass('public.{}');", table_name));
        if (result[0][0].is_null()) {
            run_query(_connection, fmt::format("CREATE TABLE {} ("
                                               "    {} bigint,"
                                               "    {} varchar,"
                                               "    {} varchar)", table_name, fields[0], fields[1], fields[2]));
        }
    }

    void TweetManager::remove() {
        run_query(_connection, fmt::format("DROP TABLE IF EXISTS {}", table_name));
    }

    std::vector<Tweet> TweetManager::all() {
        auto result = run_query(_connection, fmt::format("SELECT {}, {}, {} FROM {}", fields[0], fields[1], fields[2], table_name));
        std::vector<Tweet> ret;
        for (auto item: result) {
            ret.emplace_back(std::make_tuple(item[0].as<time_t>(), item[1].as<std::string>(), item[2].as<std::string>()));
        }
        return ret;
    }

    void TweetManager::insert(time_t timestamp, const std::string& hashtags, const std::string& text) {
        std::stringstream ss; ss << std::quoted(text, '\'', '\'');
        run_query(_connection, fmt::format("INSERT INTO {} ({}, {}, {}) values ('{}', '{}', {})",
                                           table_name, fields[0], fields[1], fields[2], timestamp, hashtags, ss.str()));
    }

    void TweetManager::insert(const std::vector<Tweet>& data) {
        std::ostringstream os;
        os << "INSERT INTO " << table_name << " (" << fields[0] << ", " << fields[1] << ", " << fields[2] << ") values ";
        for (auto it = data.begin(); it != data.end(); ++it) {
            const Tweet& tw = *it;
            if (it != data.begin()) { os << ", "; }
            os << "('" << std::get<0>(tw) << "', '" << std::get<1>(tw) << "', " << std::quoted(std::get<2>(tw), '\'', '\'') << ")";
        }
        run_query(_connection, os.str());
    }

    std::vector<Tweet> TweetManager::filter(time_t init, time_t end) {
        std::vector<Tweet> ret;
        return ret;
    }

}

#pragma once

#include "tweet.h"

namespace db {
    class Database {
    public:
        static Database& instance();
        db::TweetManager& tweets();

    protected:
        Database();
        ~Database();

        struct Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
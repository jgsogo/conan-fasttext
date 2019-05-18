
#include "database.h"

#include <pqxx/pqxx>

namespace db {

    struct Database::Impl {
        pqxx::connection _connection;
    };

    Database::Database() : pImpl(std::make_unique<Impl>()) {}
    Database::~Database() {}

    Database& Database::instance() {
        static Database instance;
        return instance;
    }

    db::TweetManager& Database::tweets(){
        static db::TweetManager manager(pImpl->_connection);
        return manager;
    }

}

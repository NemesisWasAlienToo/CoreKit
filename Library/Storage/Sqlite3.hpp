// Dependency : sqlite3, libsqlite3-dev

#pragma once

#include <iostream>
#include <string>
#include <sqlite3.h>

namespace Core
{
    namespace Storage
    {
        class Sqlite3
        {
        private:
            sqlite3 *db;

        public:
            Sqlite3(/* args */);
            ~Sqlite3();

            // Statics

            static Sqlite3 Open(const std::string &Name)
            {
                Sqlite3 ret;

                int res = sqlite3_open(Name.c_str(), &ret.db);

                if (res)
                {
                    throw std::invalid_argument(sqlite3_errmsg(ret.db));
                }

                return ret;
            }

            // Functionalities

            void Close();
        };
    }
}
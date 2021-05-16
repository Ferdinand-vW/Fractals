#pragma once

#include <sqlite3.h> 
#include <string>
#include <memory>

enum class DBType { SQLite3 };

template <DBType T> struct DBTypeToHandleType;
template<> struct DBTypeToHandleType<DBType::SQLite3> {
    using type = sqlite3;
};

template <DBType T>
class DBHandle {
    DBTypeToHandleType<T> handle;
    public:
        int open(std::string filepath);
        int close();
};


template <>
class DBHandle<DBType::SQLite3> {
    std::shared_ptr<sqlite3> handle;

    public:
        DBHandle() {};

        int open(std::string filepath) {
            sqlite3 *t_handle;
            sqlite3_open(filepath.c_str(), &t_handle);

            handle = std::shared_ptr<sqlite3>(t_handle,&sqlite3_close);

            return 0;
        }
};
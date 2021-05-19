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

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

template <>
class DBHandle<DBType::SQLite3> {
    std::shared_ptr<sqlite3> handle;

    public:
        DBHandle() {};

        int open(std::string filepath) {
            sqlite3 *t_handle;
            char *zErrMsg = 0;
            auto ret = sqlite3_open(filepath.c_str(), &t_handle);

            handle = std::shared_ptr<sqlite3>(t_handle,&sqlite3_close);

            auto sql = "CREATE TABLE COMPANY("  \
                "ID INT PRIMARY KEY     NOT NULL," \
                "NAME           TEXT    NOT NULL," \
                "AGE            INT     NOT NULL," \
                "ADDRESS        CHAR(50)," \
                "SALARY         REAL );";

            sqlite3_exec(handle.get(), sql, callback, 0, &zErrMsg);

            return ret;
        }
};
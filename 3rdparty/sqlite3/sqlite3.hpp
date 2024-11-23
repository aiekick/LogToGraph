#pragma once

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
#if defined(sqlite3_EXPORTS)
#define SQLITE_API __declspec(dllexport)
#elif defined(BUILD_SQLITE_SHARED_LIBS)
#define SQLITE_API __declspec(dllimport)
#else
#define SQLITE_API
#endif
#else
#define SQLITE_API
#endif

#include <include/sqlite3.h>
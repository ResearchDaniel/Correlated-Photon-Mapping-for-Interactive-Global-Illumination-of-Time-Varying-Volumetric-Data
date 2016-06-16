/* Copyright (c) 2014 Bruce Merry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 *
 * Abstractions for persistent caching.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <clogs/visibility_push.h>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <locale>
#include <vector>
#include <string>
#include <cstdlib>
#include <cassert>

#if CLOGS_FS_UNIX
# include <sys/stat.h>
#endif
#if CLOGS_FS_WINDOWS
# include <shlobj.h>
# include <winnls.h>
# include <shlwapi.h>
#endif

#include <clogs/visibility_pop.h>

#include "cache.h"
#include "sqlite3.h"
#include <clogs/core.h>

namespace clogs
{
namespace detail
{

namespace
{

/**
 * Determines the cache file, without caching the result. The directory is
 * created if it does not exist, but failure to create it is not reported.
 */
static std::string getCacheFileStatic();

#if CLOGS_FS_UNIX
/**
 * Retrieve an environment variable as a C++ string. If the environment
 * variable is missing, returns an empty string.
 */
static std::string getenvString(const char *name)
{
    const char *env = getenv(name);
    if (env == NULL)
        env = "";
    return env;
}

static std::string getCacheFileStatic()
{
    std::string cacheDir = getenvString("CLOGS_CACHE_DIR");
    if (cacheDir.empty())
    {
        std::string cacheHome = getenvString("XDG_CACHE_HOME");
        if (cacheHome.empty())
            cacheHome = getenvString("HOME") + "/.cache";
        mkdir(cacheHome.c_str(), 0700);
        cacheDir = cacheHome + "/clogs";
    }
    mkdir(cacheDir.c_str(), 0700);
    return cacheDir + "/cache.sqlite";
}
#endif

#if CLOGS_FS_WINDOWS
static std::string getCacheFileStatic()
{
    const wchar_t *envCacheDir = _wgetenv(L"CLOGS_CACHE_DIR");
    wchar_t path[MAX_PATH + 30] = L"";
    bool success = false;
    if (envCacheDir == NULL)
    {
        HRESULT status = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                         NULL, SHGFP_TYPE_CURRENT, path);
        if (PathAppend(path, L"clogs"))
        {
            CreateDirectory(path, NULL);
            if (PathAppend(path, L"cache"))
            {
                CreateDirectory(path, NULL);
                success = true;
            }
        }
    }
    else
    {
        success = true;
        if (wcslen(envCacheDir) < sizeof(path) / sizeof(path[0]))
        {
            wcscpy(path, envCacheDir);
            success = true;
        }
    }
    if (success && !PathAppend(path, L"cache.sqlite"))
        success = false;
    if (!success)
        path[0] = L'\0';

    int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
    if (len == 0) // indicates failure
        return "";
    std::vector<char> out(len);
    WideCharToMultiByte(CP_UTF8, 0, path, -1, &out[0], len, NULL, NULL);
    return std::string(&out[0]);
}
#endif

/**
 * Returns the cache file, caching the result after the first time.
 *
 * @see getCacheFileStatic.
 */
static std::string getCacheFile()
{
    static const std::string ans = getCacheFileStatic();
    return ans;
}

} // anonymous namespace

sqlite3_ptr::sqlite3_ptr(sqlite3 *p)
    : ptr(p)
{
}

sqlite3_ptr::~sqlite3_ptr()
{
    reset();
}

void sqlite3_ptr::reset(sqlite3 *p)
{
    if (ptr != NULL && ptr != p)
    {
        int status = sqlite3_close(ptr);
        if (status != SQLITE_OK)
        {
            std::cerr << sqlite3_errstr(status) << '\n';
        }
    }
    ptr = p;
}

sqlite3_stmt_ptr::sqlite3_stmt_ptr(sqlite3_stmt *p)
: ptr(p)
{
}

sqlite3_stmt_ptr::~sqlite3_stmt_ptr()
{
    reset();
}

void sqlite3_stmt_ptr::reset(sqlite3_stmt *p)
{
    if (ptr != NULL && ptr != p)
    {
        int status = sqlite3_finalize(ptr);
        if (status != SQLITE_OK)
        {
            std::cerr << sqlite3_errstr(status) << '\n';
        }
    }
    ptr = p;
}


template<typename T>
static void writeFieldDefinitions(std::ostream &statement)
{
    std::vector<const char *> names, types;
    fieldNames((T *) NULL, NULL, names);
    fieldTypes((T *) NULL, types);
    assert(names.size() == types.size());
    for (std::size_t i = 0; i < names.size(); i++)
        statement << names[i] << ' ' << types[i] << ", ";
}

template<typename T>
static void writeFieldNames(std::ostream &statement, const char *sep = ", ", const char *suffix = "")
{
    std::vector<const char *> names;
    fieldNames((T *) NULL, NULL, names);
    for (std::size_t i = 0; i < names.size(); i++)
    {
        if (i > 0)
            statement << sep;
        statement << names[i] << suffix;
    }
}

template<typename K, typename V>
void Table<K, V>::createTable(const char *name)
{
    std::ostringstream statement;
    statement.imbue(std::locale::classic());
    statement << "CREATE TABLE IF NOT EXISTS " << name << " (";
    writeFieldDefinitions<K>(statement);
    writeFieldDefinitions<V>(statement);

    statement << "PRIMARY KEY(";
    writeFieldNames<K>(statement);
    statement << "))";

    const std::string s = statement.str();
    char *err = NULL;
    int status = sqlite3_exec(con, s.c_str(), NULL, NULL, &err);
    if (status != SQLITE_OK || err != NULL)
    {
        CacheError error(s + ": " + err);
        sqlite3_free(err);
        throw error;
    }
}

template<typename K, typename V>
void Table<K, V>::prepareAdd(const char *name)
{
    std::ostringstream statement;
    statement.imbue(std::locale::classic());
    statement << "INSERT OR REPLACE INTO " << name << "(";
    writeFieldNames<K>(statement);
    statement << ", ";
    writeFieldNames<V>(statement);

    statement << ") VALUES (";
    std::vector<const char *> names;
    fieldNames((K *) NULL, NULL, names);
    fieldNames((V *) NULL, NULL, names);
    for (std::size_t i = 0; i < names.size(); i++)
    {
        if (i > 0)
            statement << ", ";
        statement << '?';
    }
    statement << ')';

    sqlite3_stmt *stmt = NULL;
    int status = sqlite3_prepare_v2(con, statement.str().c_str(), -1, &stmt, NULL);
    addStmt.reset(stmt);
    if (status != SQLITE_OK)
        throw CacheError(sqlite3_errstr(status));
}

template<typename K, typename V>
void Table<K, V>::prepareQuery(const char *name)
{
    std::ostringstream query;
    query.imbue(std::locale::classic());
    query << "SELECT ";
    writeFieldNames<V>(query);

    query << " FROM " << name << " WHERE ";
    writeFieldNames<K>(query, " AND ", "=?");

    sqlite3_stmt *stmt = NULL;
    int status = sqlite3_prepare_v2(con, query.str().c_str(), -1, &stmt, NULL);
    queryStmt.reset(stmt);
    if (status != SQLITE_OK)
        throw CacheError(sqlite3_errstr(status));
}

template<typename K, typename V>
void Table<K, V>::add(const K &key, const V &value)
{
    sqlite3_reset(addStmt.get());

    int pos = 1;
    pos = bindFields(addStmt.get(), pos, key);
    pos = bindFields(addStmt.get(), pos, value);

    int status = sqlite3_step(addStmt.get());
    if (status != SQLITE_DONE)
        throw CacheError(sqlite3_errstr(status));
}

template<typename K, typename V>
bool Table<K, V>::lookup(const K &key, V &value) const
{
    sqlite3_reset(queryStmt.get());

    bindFields(queryStmt.get(), 1, key);
    int status = sqlite3_step(queryStmt.get());
    if (status == SQLITE_DONE)
        return false;
    else if (status != SQLITE_ROW)
        throw CacheError(sqlite3_errstr(status));
    else
    {
        readFields(queryStmt.get(), 0, value);
        return true;
    }
}

template<typename K, typename V>
Table<K, V>::Table(sqlite3 *con, const char *name)
    : con(con)
{
    createTable(name);
    prepareAdd(name);
    prepareQuery(name);
}


sqlite3 *DB::open()
{
    sqlite3 *c = NULL;
    const std::string filename = getCacheFile();
    int status = sqlite3_open_v2(
        filename.c_str(), &c,
        SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        NULL);
    if (status != SQLITE_OK)
    {
        if (c != NULL)
            sqlite3_close(c);
        throw CacheError(sqlite3_errstr(status));
    }
    return c;
}

DB::DB() :
    con(open()),
    scan(con.get(), ScanParameters::tableName()),
    reduce(con.get(), ReduceParameters::tableName()),
    radixsort(con.get(), RadixsortParameters::tableName()),
    kernel(con.get(), KernelParameters::tableName())
{
}

CLOGS_LOCAL DB &getDB()
{
    static DB db;
    return db;
}

template class Table<ScanParameters::Key, ScanParameters::Value>;
template class Table<ReduceParameters::Key, ReduceParameters::Value>;
template class Table<RadixsortParameters::Key, RadixsortParameters::Value>;
template class Table<KernelParameters::Key, KernelParameters::Value>;

} // namespace detail
} // namespace clogs

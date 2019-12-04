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

#ifndef CACHE_H
#define CACHE_H

#include <clogs/visibility_push.h>
#include <cstddef>
#include <boost/noncopyable.hpp>
#include <clogs/visibility_pop.h>

#include "cache_types.h"
#include "parameters.h"

struct sqlite3;
struct sqlite3_stmt;

namespace clogs
{
namespace detail
{

/// RAII wrapper around sqlite3*.
class CLOGS_LOCAL sqlite3_ptr : public boost::noncopyable
{
private:
    sqlite3 *ptr;

public:
    explicit sqlite3_ptr(sqlite3 *p = NULL);
    ~sqlite3_ptr();

    void reset(sqlite3 *p = NULL);
    sqlite3 *get() const { return ptr; }
};

/// RAII wrapper around sqlite3_stmt*.
class CLOGS_LOCAL sqlite3_stmt_ptr : public boost::noncopyable
{
private:
    sqlite3_stmt *ptr;

public:
    explicit sqlite3_stmt_ptr(sqlite3_stmt *p = NULL);
    ~sqlite3_stmt_ptr();

    void reset(sqlite3_stmt *p = NULL);
    sqlite3_stmt *get() const { return ptr; }
};

/**
 * Abstraction of a table supporting insertion of one row at a time, and
 * lookup using the primary key. The key type @a K and value type @a V
 * should be declared with #CLOGS_STRUCT.
 *
 * Column names are used without any quoting, so they must not use any SQL
 * keywords.
 */
template<typename K, typename V>
class CLOGS_LOCAL Table
{
private:
    sqlite3 *con;
    sqlite3_stmt_ptr addStmt, queryStmt;

    /// Create the table if it does not exist
    void createTable(const char *name);
    /// Create the prepared statement for insertions
    void prepareAdd(const char *name);
    /// Create the prepared statement for queries
    void prepareQuery(const char *name);

public:
    /**
     * Constructor. The provided @a con must not be closed before
     * this object is destroyed.
     *
     * @param con      Connection to the database
     * @param table    The name for the table
     */
    Table(sqlite3 *con, const char *table);

    /// Insert a record into the table, replacing any previous one with the same key
    void add(const K &key, const V &value);

    /**
     * Find a record in the table.
     *
     * @param key         Lookup key
     * @param[out] value  Found value, if any
     * @return whether the record was found
     */
    bool lookup(const K &key, V &value) const;
};

/**
 * Class for connection to the database. There is only ever one instance of
 * this class, which handles initialization and shutdown.
 */
class CLOGS_LOCAL DB
{
private:
    sqlite3_ptr con;
    static sqlite3 *open(); ///< Open the database connection (used by constructor).

public:
    Table<ScanParameters::Key, ScanParameters::Value> scan;
    Table<ReduceParameters::Key, ReduceParameters::Value> reduce;
    Table<RadixsortParameters::Key, RadixsortParameters::Value> radixsort;
    Table<KernelParameters::Key, KernelParameters::Value> kernel;

    DB();
};

/// Retrieve a singleton database instance
CLOGS_LOCAL DB &getDB();

} // namespace detail
} // namespace clogs

#endif /* CACHE_H */

/* Copyright (c) 2012-2014 University of Cape Town
 * Copyright (c) 2014 Bruce Merry
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
 * Utilities for passing around generic sets of key/value parameters.
 */

#include <clogs/visibility_push.h>
#include <string>
#include <istream>
#include <ostream>
#include <sstream>
#include <map>
#include <locale>
#include <memory>
#include <clogs/visibility_pop.h>

#include "parameters.h"

namespace clogs
{
namespace detail
{

CLOGS_LOCAL int bindFields(sqlite3_stmt *stmt, int pos, sqlite3_int64 value)
{
    int status = sqlite3_bind_int64(stmt, pos, value);
    if (status != SQLITE_OK)
        throw CacheError(sqlite3_errstr(status));
    return pos + 1;
}

CLOGS_LOCAL int bindFields(sqlite3_stmt *stmt, int pos, const std::string &value)
{
    int status = sqlite3_bind_text(stmt, pos, value.data(), value.size(), SQLITE_TRANSIENT);
    if (status != SQLITE_OK)
        throw CacheError(sqlite3_errstr(status));
    return pos + 1;
}

CLOGS_LOCAL int bindFields(sqlite3_stmt *stmt, int pos, const std::vector<unsigned char> &value)
{
    const unsigned char dummy = 0;
    int status = sqlite3_bind_blob(
        stmt, pos,
        static_cast<const void *>(value.empty() ? &dummy : &value[0]),
        value.size(), SQLITE_TRANSIENT);
    if (status != SQLITE_OK)
        throw CacheError(sqlite3_errstr(status));
    return pos + 1;
}


CLOGS_LOCAL int readFields(sqlite3_stmt *stmt, int pos, std::string &value)
{
    assert(pos >= 0 && pos < sqlite3_column_count(stmt));
    assert(sqlite3_column_type(stmt, pos) == SQLITE_TEXT);
    const char *data = (const char *) sqlite3_column_text(stmt, pos);
    ::size_t size = sqlite3_column_bytes(stmt, pos);
    value.assign(data, size);
    return pos + 1;
}

CLOGS_LOCAL int readFields(sqlite3_stmt *stmt, int pos, std::vector<unsigned char> &value)
{
    assert(pos >= 0 && pos < sqlite3_column_count(stmt));
    assert(sqlite3_column_type(stmt, pos) == SQLITE_BLOB);
    const unsigned char *data = (const unsigned char *) sqlite3_column_blob(stmt, pos);
    ::size_t size = sqlite3_column_bytes(stmt, pos);
    value.assign(data, data + size);
    return pos + 1;
}

} // namespace detail
} // namespace clogs

#ifndef _rtdb_test_wide_base_h_
#define _rtdb_test_wide_base_h_

#define RTDB_wide_TXT_DB_LEAD    "#DB,"

// Use tsdb_v3_t's tools member variable, we can use 5 hundreds of cross platform tool functions.
#include <tsdb_v3.h>
// RTDB Cross platform Tool functions.
#include <tsdb_tools.h>

#define ENABLE_TDENGINE     1

#if defined (_WIN64) || defined (__linux__)
#define ENABLE_TIMESCALEDB     1
#endif

namespace rtdb
{
namespace test
{
namespace wide
{

enum db_type_t
{
    DB_UNKNOWN  = 0,

    DB_RTDB     = 1,

    DB_TAOS     = 2,

    DB_TIMESCALEDB = 3,

    #define DB_FIRST    DB_RTDB
    #define DB_LAST     DB_TIMESCALEDB
};

bool        rtdb_init( const char * path );
tsdb_v3_t * rtdb_tls();
tsdb_v3_t * rtdb_new();
db_type_t get_db_type( int argc, char ** argv );

struct wide_conn_t
{
public:

    virtual void kill_me() = 0;

    virtual int select_db( const char * db ) = 0;

    virtual int query_non_result( const char * sql, size_t sql_len ) = 0;

    virtual int query_has_result( const char * sql, size_t sql_len, uint64_t & row_count ) = 0;

protected:
    wide_conn_t() {}
    virtual ~wide_conn_t() {}

private:
    // disable
    wide_conn_t(const wide_conn_t &);
    const wide_conn_t & operator = (const wide_conn_t &);
};

struct wide_base_t
{
public:

    static wide_base_t * instance( db_type_t type, int argc, char ** argv);

    virtual db_type_t get_type() = 0;

    virtual int global_init( int argc, char ** argv ) = 0;

    virtual int global_terminate() = 0;

    virtual int global_connect( const char * server ) = 0;

    virtual wide_conn_t * create_conn() = 0;

protected:

    wide_base_t() {}
    virtual ~wide_base_t() {}

private:
    // disable
    wide_base_t(const wide_base_t &);
    const wide_base_t & operator = (const wide_base_t &);
};

} // namespace wide
} // namespace test
} // namespace rtdb

#endif

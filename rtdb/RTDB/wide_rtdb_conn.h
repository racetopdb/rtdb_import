#ifndef _rtdb_test_wide_rtdb_conn_h_
#define _rtdb_test_wide_rtdb_conn_h_

#include "../wide_base.h"

namespace rtdb
{

namespace wide
{

struct wide_rtdb_conn_t : public wide_conn_t
{
public:
    virtual void kill_me();

    virtual int select_db( const char * db );

    virtual int query_non_result( const char * sql, size_t sql_len );

    virtual int query_has_result( const char * sql, size_t sql_len, uint64_t & row_count );

public:
    wide_rtdb_conn_t();
    virtual ~wide_rtdb_conn_t();

private:
    // disable
    wide_rtdb_conn_t(const wide_rtdb_conn_t &);
    const wide_rtdb_conn_t & operator = (const wide_rtdb_conn_t &);
};

} // namespace wide

} // namespace rtdb

#endif

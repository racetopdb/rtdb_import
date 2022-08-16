#ifndef _rtdb_test_clickhouse_conn_h_
#define _rtdb_test_clickhouse_conn_h_

#include "../wide_base.h"

#if ENABLE_CLICKHOUSE
#include <clickhouse/client.h>

namespace rtdb
{

namespace wide
{

struct wide_clickhouse_conn_t : public wide_conn_t
{
public:
    virtual void kill_me();

    virtual int select_db( const char * db );

    virtual int query_non_result( const char * sql, size_t sql_len );

    virtual int query_has_result( const char * sql, size_t sql_len, uint64_t & row_count );

public:
    wide_clickhouse_conn_t();
    virtual ~wide_clickhouse_conn_t();

    int connect( const char * server );
    void disconnect();

private:

    clickhouse::Client* m_conn;
    

    // 查询语句  
    std::string m_qureysql;

private:

    // disable
    wide_clickhouse_conn_t(const wide_clickhouse_conn_t &);
    const wide_clickhouse_conn_t & operator = (const wide_clickhouse_conn_t &);
};

} // namespace wide

} // namespace rtdb
#endif // #if ENABLE_CLICKHOUSE

#endif

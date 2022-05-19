#ifndef _rtdb_test_wide_taos_conn_h_
#define _rtdb_test_wide_taos_conn_h_

#include "../wide_base.h"

#if ENABLE_TIMESCALEDB
#include <libpq-fe.h>
#endif // #if ENABLE_TIMESCALEDB


namespace rtdb
{
namespace test
{
namespace wide
{

struct wide_timescaledb_conn_t : public wide_conn_t
{
public:
    virtual void kill_me();

    virtual int select_db( const char * db );

    virtual int query_non_result( const char * sql, size_t sql_len );

    virtual int query_has_result( const char * sql, size_t sql_len, uint64_t & row_count );

public:
    wide_timescaledb_conn_t();
    virtual ~wide_timescaledb_conn_t();

    int connect( const char * server );
    void disconnect();

private:

    PGconn*  m_conn;
    // 连接信息 类似   
    std::string m_conninfo;

    // 查询语句  
    std::string m_qureysql;

private:
    // 是否启用 s_m_is_timescaledb_enable 默认 false  
    static volatile bool s_m_is_timescaledb_enable;

    // 锁  
    static struct lockable_t s_m_lockable;

    // disable
    wide_timescaledb_conn_t(const wide_timescaledb_conn_t &);
    const wide_timescaledb_conn_t & operator = (const wide_timescaledb_conn_t &);
};

} // namespace wide
} // namespace test
} // namespace rtdb

#endif

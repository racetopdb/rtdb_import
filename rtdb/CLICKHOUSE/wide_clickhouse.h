#ifndef _rtdb_test_wide_clickhouse_h_
#define _rtdb_test_wide_clickhouse_h_

#include "../wide_base.h"

#if ENABLE_CLICKHOUSE

namespace rtdb
{

namespace wide
{

struct wide_clickhouse_t : public wide_base_t
{
public:
    wide_clickhouse_t();
    virtual ~wide_clickhouse_t();

    db_type_t get_type() { return DB_CLICKHOUSE; }

    virtual int global_init( int argc, char ** argv );

    virtual int global_terminate();

    virtual int global_connect( const char * server );

    virtual wide_conn_t * create_conn();

private:

    std::string     m_server;

private:
    // disable
    wide_clickhouse_t(const wide_clickhouse_t &);
    const wide_clickhouse_t & operator = (const wide_clickhouse_t &);
};

} // namespace wide

} // namespace rtdb

#endif // #if ENABLE_CLICKHOUSE

#endif

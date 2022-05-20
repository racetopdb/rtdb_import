#ifndef _rtdb_test_wide_timescaledb_h_
#define _rtdb_test_wide_timescaledb_h_

#include "../wide_base.h"

#if ENABLE_TIMESCALEDB

namespace rtdb
{

namespace wide
{

struct wide_timescaledb_t : public wide_base_t
{
public:
    wide_timescaledb_t();
    virtual ~wide_timescaledb_t();

    db_type_t get_type() { return DB_TIMESCALEDB; }

    virtual int global_init( int argc, char ** argv );

    virtual int global_terminate();

    virtual int global_connect( const char * server );

    virtual wide_conn_t * create_conn();

private:

    std::string     m_server;

private:
    // disable
    wide_timescaledb_t(const wide_timescaledb_t &);
    const wide_timescaledb_t & operator = (const wide_timescaledb_t &);
};

} // namespace wide

} // namespace rtdb

#endif // #if ENABLE_TIMESCALEDB

#endif

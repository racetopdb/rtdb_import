#ifndef _rtdb_test_wide_rtdb_h_
#define _rtdb_test_wide_rtdb_h_

#include "../wide_base.h"
#include "wide_rtdb_conn.h"

namespace rtdb
{
namespace test
{
namespace wide
{

struct wide_rtdb_t : public wide_base_t
{
public:
    wide_rtdb_t();
    virtual ~wide_rtdb_t();

    db_type_t get_type() { return DB_RTDB; }

    virtual int global_init( int argc, char ** argv );

    virtual int global_terminate() { return 0; }

    virtual int global_connect( const char * server );

    virtual wide_conn_t * create_conn();

private:

    int setup_timeout( int argc, char ** argv );

    int server_disk_usage_percent( int & result );

private:

    wide_rtdb_conn_t  m_conn;

private:
    // disable
    wide_rtdb_t(const wide_rtdb_t &);
    const wide_rtdb_t & operator = (const wide_rtdb_t &);
};

} // namespace wide
} // namespace test
} // namespace rtdb

#endif

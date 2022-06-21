#ifndef _rtdb_test_wide_influxdb_h_
#define _rtdb_test_wide_influxdb_h_

#include "../wide_base.h"

namespace rtdb
{

namespace wide
{

struct wide_influxdb_t : public wide_base_t
{
public:
    wide_influxdb_t();
    virtual ~wide_influxdb_t();

    db_type_t get_type() { return DB_INFLUXDB; }

    virtual int global_init( int argc, char ** argv );

    virtual int global_terminate();

    virtual int global_connect( const char * server );

    virtual wide_conn_t * create_conn();

private:

    std::string     m_server;

private:
    // disable
    wide_influxdb_t(const wide_influxdb_t &);
    const wide_influxdb_t & operator = (const wide_influxdb_t &);
};

} // namespace wide

} // namespace rtdb

#endif

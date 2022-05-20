#ifndef _rtdb_test_wide_taos_h_
#define _rtdb_test_wide_taos_h_

#include "../wide_base.h"

namespace rtdb
{

namespace wide
{

struct wide_taos_t : public wide_base_t
{
public:
    wide_taos_t();
    virtual ~wide_taos_t();

    db_type_t get_type() { return DB_TAOS; }

    virtual int global_init( int argc, char ** argv );

    virtual int global_terminate();

    virtual int global_connect( const char * server );

    virtual wide_conn_t * create_conn();

private:

    std::string     m_server;

private:
    // disable
    wide_taos_t(const wide_taos_t &);
    const wide_taos_t & operator = (const wide_taos_t &);
};

} // namespace wide

} // namespace rtdb

#endif

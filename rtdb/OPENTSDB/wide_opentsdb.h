#ifndef _rtdb_test_wide_opentsdb_h_
#define _rtdb_test_wide_opentsdb_h_

#include "../wide_base.h"

namespace rtdb
{

namespace wide
{

// opentsdb 最大的字段数   
#define MAX_OPENTSDB_FILES_COUNT 9
struct wide_opentsdb_t : public wide_base_t
{
public:
    wide_opentsdb_t();
    virtual ~wide_opentsdb_t();

    db_type_t get_type() { return DB_OPENTSDB; }

    virtual int global_init( int argc, char ** argv );

    virtual int global_terminate();

    virtual int global_connect( const char * server );

    virtual wide_conn_t * create_conn();

private:

    std::string     m_server;

private:
    // disable
    wide_opentsdb_t(const wide_opentsdb_t &);
    const wide_opentsdb_t & operator = (const wide_opentsdb_t &);
};

} // namespace wide

} // namespace rtdb

#endif

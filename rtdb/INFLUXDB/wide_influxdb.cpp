#include "wide_influxdb.h"
#include "wide_influxdb_conn.h"


#if ENABLE_INFLUXDB
#include "../HTTP.h"    // INFLUXDB Database Interface.
#endif // #if ENABLE_INFLUXDB

namespace rtdb
{

namespace wide
{

wide_influxdb_t::wide_influxdb_t()
    : m_server()
{
}

wide_influxdb_t::~wide_influxdb_t()
{
}

int wide_influxdb_t::global_init( int argc, char ** argv )
{
#if ENABLE_INFLUXDB
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r;
    
    uint64_t start = p->tools->get_tick_count_us();
    r = HTTP::init_once();
    uint64_t stop = p->tools->get_tick_count_us();
    uint64_t span = stop - start;
    if ( 0 == r ) {
        TSDB_INFO( p, "[influxdb][use=%lld us]HTTP::init_once OK", (long long)span );
    } else {
        TSDB_ERROR( p, "[influxdb][r=%d][use=%lld us]HTTP::init_once failed", r, (long long)span );
    }

    return r;
#else
    return ENOSYS;
#endif
}

int wide_influxdb_t::global_terminate()
{
#if ENABLE_INFLUXDB
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    uint64_t start = p->tools->get_tick_count_us();
    HTTP::destroy_once();
    uint64_t stop = p->tools->get_tick_count_us();
    uint64_t span = stop - start;

    TSDB_INFO( p, "[influxdb][use=%lld us]HTTP::destroy_once OK", (long long)span );
    return 0;
#else
    return ENOSYS;
#endif
}

int wide_influxdb_t::global_connect( const char * server )
{
    m_server = "http://";
    m_server += server;
    return 0;
}

wide_conn_t * wide_influxdb_t::create_conn()
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    wide_influxdb_conn_t * conn = new wide_influxdb_conn_t();
    int r = conn->connect( m_server.c_str() );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[influxdb][r=%d][server=%s]connect failed", r, m_server.c_str() );
        delete conn;
        return NULL;
    }

    return conn;
}

} // namespace wide

} // namespace rtdb

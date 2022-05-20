#include "wide_timescaledb.h"
#include "wide_timescaledb_conn.h"

#if ENABLE_TIMESCALEDB

#include <libpq-fe.h>


namespace rtdb
{

namespace wide
{

wide_timescaledb_t::wide_timescaledb_t()
    : m_server()
{
}

wide_timescaledb_t::~wide_timescaledb_t()
{
}

int wide_timescaledb_t::global_init( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r = 0 ;

    return r;
}

int wide_timescaledb_t::global_terminate()
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    return 0;
}

int wide_timescaledb_t::global_connect( const char * server )
{
    // TDEngine does not support global connect
    m_server = server;
    return 0;
}

wide_conn_t * wide_timescaledb_t::create_conn()
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    wide_timescaledb_conn_t * conn = new wide_timescaledb_conn_t();
    int r = conn->connect( m_server.c_str() );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[TIMESCALEDB][r=%d][server=%s]connect failed", r, m_server.c_str() );
        delete conn;
        return NULL;
    }

    return conn;
}

#ifdef _MSC_VER
#pragma message( "build TIMESCALEDB test wide table on X86_64" )
#pragma comment( lib, "TIMESCALEDB/x64/libpq.lib" )
#endif

} // namespace wide

} // namespace rtdb
#endif // #if ENABLE_TIMESCALEDB
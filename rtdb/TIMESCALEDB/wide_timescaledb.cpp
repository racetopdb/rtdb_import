#include "wide_timescaledb.h"
#include "wide_timescaledb_conn.h"

#if ENABLE_TIMESCALEDB

#include <libpq-fe.h>

#endif // #if ENABLE_TIMESCALEDB

namespace rtdb
{
namespace test
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
#if ENABLE_TIMESCALEDB
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r = 0 ;

    return r;
#else
    return ENOSYS;
#endif
}

int wide_timescaledb_t::global_terminate()
{
#if ENABLE_TIMESCALEDB
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    return 0;
#else
    return ENOSYS;
#endif
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

#if ENABLE_TIMESCALEDB
#ifdef _MSC_VER
    #if defined( _WIN64 )
        #pragma message( "build TIMESCALEDB test wide table on X86_64" )
        #pragma comment( lib, "TIMESCALEDB/x64/libpq.lib" )
    #elif defined( _WIN32 )
        #error "build TIMESCALEDB test wide table on Win32 not Support!!!"
    #endif
#endif
#endif // #if ENABLE_TIMESCALEDB

} // namespace wide
} // namespace test
} // namespace rtdb

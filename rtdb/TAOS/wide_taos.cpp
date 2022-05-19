#include "wide_taos.h"
#include "wide_taos_conn.h"

#if ENABLE_TDENGINE

#include <taos.h>           // TDEngine Database Interface.
#include <taoserror.h>

#endif // #if ENABLE_TDENGINE

namespace rtdb
{
namespace test
{
namespace wide
{

wide_taos_t::wide_taos_t()
    : m_server()
{
}

wide_taos_t::~wide_taos_t()
{
}

int wide_taos_t::global_init( int argc, char ** argv )
{
#if ENABLE_TDENGINE
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r;
    
    uint64_t start = p->tools->get_tick_count_us();
    r = taos_init();
    uint64_t stop = p->tools->get_tick_count_us();
    uint64_t span = stop - start;
    if ( 0 == r ) {
        TSDB_INFO( p, "[TAOS][use=%lld us]taos_init OK", (long long)span );
    } else {
        TSDB_ERROR( p, "[TAOS][r=%d][use=%lld us]taos_init failed", r, (long long)span );
    }

    return r;
#else
    return ENOSYS;
#endif
}

int wide_taos_t::global_terminate()
{
#if ENABLE_TDENGINE
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    uint64_t start = p->tools->get_tick_count_us();
    taos_cleanup();
    uint64_t stop = p->tools->get_tick_count_us();
    uint64_t span = stop - start;

    TSDB_INFO( p, "[TAOS][use=%lld us]taos_cleanup OK", (long long)span );
    return 0;
#else
    return ENOSYS;
#endif
}

int wide_taos_t::global_connect( const char * server )
{
    // TDEngine does not support global connect
    m_server = server;
    return 0;
}

wide_conn_t * wide_taos_t::create_conn()
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    wide_taos_conn_t * conn = new wide_taos_conn_t();
    int r = conn->connect( m_server.c_str() );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[TAOS][r=%d][server=%s]connect failed", r, m_server.c_str() );
        delete conn;
        return NULL;
    }

    return conn;
}

#if ENABLE_TDENGINE
#ifdef _MSC_VER
    #if defined( _WIN64 )
        #pragma message( "build TAOS test wide table on X86_64" )
        #pragma comment( lib, "TAOS/x64/taos.lib" )
    #elif defined( _WIN32 )
        #pragma message( "build TAOS test wide table on Win32" )
        #pragma comment( lib, "TAOS/win32/taos.lib" )
    #endif
#endif
#endif // #if ENABLE_TDENGINE

} // namespace wide
} // namespace test
} // namespace rtdb

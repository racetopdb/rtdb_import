#include <sstream>
#include "wide_clickhouse.h"
#include "wide_clickhouse_conn.h"

#if ENABLE_CLICKHOUSE

namespace rtdb
{

namespace wide
{

wide_clickhouse_t::wide_clickhouse_t()
    : m_server()
{
}

wide_clickhouse_t::~wide_clickhouse_t()
{
}

int wide_clickhouse_t::global_init( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r = 0 ;

    return r;
}

int wide_clickhouse_t::global_terminate()
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    return 0;
}

int wide_clickhouse_t::global_connect( const char * server )
{
    // TDEngine does not support global connect
    m_server = server;
    return 0;
}

wide_conn_t * wide_clickhouse_t::create_conn()
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    wide_clickhouse_conn_t * conn = new wide_clickhouse_conn_t();
    int r = conn->connect( m_server.c_str() );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[clickhouse][r=%d][server=%s]connect failed", r, m_server.c_str() );
        delete conn;
        return NULL;
    }

    return conn;
}

#ifdef _MSC_VER
#pragma message( "build clickhouse test wide table on X86_64" )
#if _DEBUG
#pragma comment( lib, "clickhouse/x64/Debug/absl-lib.lib" )
#pragma comment( lib, "clickhouse/x64/Debug/cityhash-lib.lib" )
#pragma comment( lib, "clickhouse/x64/Debug/clickhouse-cpp-lib-static.lib" )
#pragma comment( lib, "clickhouse/x64/Debug/lz4-lib.lib" )
#else
#pragma comment( lib, "clickhouse/x64/Release/absl-lib.lib" )
#pragma comment( lib, "clickhouse/x64/Release/cityhash-lib.lib" )
#pragma comment( lib, "clickhouse/x64/Release/clickhouse-cpp-lib-static.lib" )
#pragma comment( lib, "clickhouse/x64/Release/lz4-lib.lib" )
#endif

#endif

} // namespace wide

} // namespace rtdb
#endif // #if ENABLE_CLICKHOUSE

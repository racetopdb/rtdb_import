#include "wide_rtdb.h"

// dev_li version: disable this function
#define ENABLE_RTDB_DISK_USAGE_PERCENT      0

namespace rtdb
{
namespace test
{
namespace wide
{

wide_rtdb_t::wide_rtdb_t()
    : m_conn()
{
}

wide_rtdb_t::~wide_rtdb_t()
{
}

int wide_rtdb_t::setup_timeout( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    const char * s;

    int conn_timeout_ms = 0;
    p->tools->find_argv( argc, argv, "timeout.conn", & s, NULL );
    if ( s && 0 == stricmp( "infinite", s ) ) {
        conn_timeout_ms = INFINITE;
    } else if ( s && * s > 0 && isdigit(*s) ) {
        conn_timeout_ms = atoi( s );
        if ( conn_timeout_ms <= 0 ) {
            conn_timeout_ms = 3000;
        }
    } else {
        conn_timeout_ms = 3000;
    }

    int send_timeout_ms = 0;
    p->tools->find_argv( argc, argv, "timeout.send", & s, NULL );
    if ( s && 0 == stricmp( "infinite", s ) ) {
        send_timeout_ms = INFINITE;
    } else if ( s && * s > 0 && isdigit(*s) ) {
        send_timeout_ms = atoi( s );
        if ( send_timeout_ms <= 0 ) {
            send_timeout_ms = INFINITE;
        }
    } else {
        send_timeout_ms = INFINITE;
    }

    int recv_timeout_ms = 0;
    p->tools->find_argv( argc, argv, "timeout.recv", & s, NULL );
    if ( s && 0 == stricmp( "infinite", s ) ) {
        recv_timeout_ms = INFINITE;
    } else if ( s && * s > 0 && isdigit(*s) ) {
        recv_timeout_ms = atoi( s );
        if ( recv_timeout_ms <= 0 ) {
            recv_timeout_ms = 3000;
        }
    } else {
        recv_timeout_ms = 3000;
    }

    int r = p->set_timeout( conn_timeout_ms, send_timeout_ms, recv_timeout_ms );
    if ( unlikely( 0 != r ) ) {
        TSDB_ERROR( p, "[RTDB][r=%d][conn_timeout=%d][send_timeout=%d][recv_timeout=%d]set_timeout failed",
                       r, conn_timeout_ms, send_timeout_ms, recv_timeout_ms );
        return r;
    }

    return 0;
}

int wide_rtdb_t::global_init( int argc, char ** argv )
{
    int r;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // setup timeout info
    r = setup_timeout( argc, argv );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[RTDB][r=%d]setup_timeout failed", r );
        return r;
    }

    return 0;
}

int wide_rtdb_t::server_disk_usage_percent( int & result )
{
#if ENABLE_RTDB_DISK_USAGE_PERCENT
    const char sql[] = "select disk";
    int r;
    tsdb_v3_reader_t * reader = tsdb->query_reader( tsdb, sql, (int)sizeof(sql)-1, NULL, NULL, TRUE, & r );
    if ( 0 != r || NULL == reader ) {
        TSDB_ERROR( tsdb, "[RTDB][r=%d][reader=%p]server invalid", r, reader );
        if ( 0 == r ) {
            r = EFAULT;
        }
        result = -1;
        return r;
    }
    int * disk_usage = reader->get_int_s( reader, "value" );
    if ( unlikely( NULL == disk_usage ) ) {
        TSDB_ERROR( tsdb, "[RTDB]get disk value failed, value is NULL" );
        result = -1;
        return EFAULT;
    }

    result = * disk_usage;
    return 0;
#else
    result = 0;
    return 0;
#endif // #if ENABLE_RTDB_DISK_USAGE_PERCENT
}

int wide_rtdb_t::global_connect( const char * server )
{
    if ( ! rtdb_init( NULL ) ) {
#if defined( _WIN32 ) || defined( _WIN64 )
        DWORD e = GetLastError();
        fprintf( stderr, "[GetLastError=%d]rtdb_init failed !!!!\n", (int)e );
#else
        const char * s = dlerror();
        fprintf( stderr, "[dlerror=%s]rtdb_init failed !!!!\n", s );
#endif
        return EFAULT;
    }

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // RTDB, only need to login one time.

    // Connect the RTDB server
    std::string cnstr = "user=test;passwd=test;server=tcp://";
    cnstr += server;
    int r = p->connect( cnstr.c_str() );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[RTDB][CONNECT][server=%s]connect failed", server );
        return r;
    }

#if ENABLE_RTDB_DISK_USAGE_PERCENT

    // Check RTDB server side free disk space.
    int disk_usage;
    r = server_disk_usage_percent( p, disk_usage );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[RTDB][CONNECT][server=%s][r=%d]server_disk_usage failed", server, r );
        return r;
    }
    // if disk usage >= 95%，we can not create the DB or tables.
    if ( disk_usage < 0 || disk_usage >= 95 ) {
        TSDB_ERROR( p, "[RTDB][CONNECT][server=%s] SERVER DISK FULL !!!!!!", server );
        return ENOSPC;
    }

#endif // #if ENABLE_RTDB_DISK_USAGE_PERCENT

    return 0;
}

wide_conn_t * wide_rtdb_t::create_conn()
{
    return & m_conn;
}

} // namespace wide
} // namespace test
} // namespace rtdb

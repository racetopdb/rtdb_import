#include "wide_taos_conn.h"

#if ENABLE_TDENGINE

#include <taos.h>           // TDEngine Database Interface.
#include <taoserror.h>

#endif // #if ENABLE_TDENGINE

#define TAOS_USER   "root"
#define TAOS_PASSWD "taosdata"

namespace rtdb
{
namespace test
{
namespace wide
{

wide_taos_conn_t::wide_taos_conn_t()
    : m_conn( NULL )
{
}

wide_taos_conn_t::~wide_taos_conn_t()
{
    disconnect();
}

void wide_taos_conn_t::kill_me()
{
    delete this;
}

int wide_taos_conn_t::connect( const char * server )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    sockaddr_in addr;
    if ( ! p->tools->socket_str_2_addr_v4( server, & addr ) ) {
        TSDB_ERROR( p, "[TAOS][server=%s]invalid server string", server );
        return EINVAL;
    }

    disconnect();
    m_conn = taos_connect( inet_ntoa(addr.sin_addr), TAOS_USER, TAOS_PASSWD, "", ntohs(addr.sin_port) );
    if ( NULL == m_conn ) {
        TSDB_ERROR( p, "[TAOS][server=%s]taos_connect failed", server );
        return ENETUNREACH;
    }

    return 0;
}

void wide_taos_conn_t::disconnect()
{
    if ( m_conn ) {
        taos_close( m_conn );
        m_conn = NULL;
    }
}

int wide_taos_conn_t::select_db( const char * db )
{
    int r = taos_select_db( m_conn, db );
    if ( unlikely( 0 != r ) ) {
        // get RTDB interface from TLS(thread local storage).
        // We strongly recommend that you only call this function where you need it,
        // and without storage the pointer for later use.
        tsdb_v3_t * p = rtdb_tls();
        assert( p );

        TSDB_ERROR( p, "[TAOS][r=%d][db=%s] taos_select_db failed", r, db );
        return r;
    }

    return 0;
}

int wide_taos_conn_t::query_non_result( const char * sql, size_t sql_len )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    TAOS_RES* res = taos_query( m_conn, sql );
    if ( NULL == res ) {
        TSDB_ERROR( p, "[TAOS]taos_query failed, SQL is coming..." );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return EFAULT;
    }

    int r = taos_errno( res );

    if ( 0 != r ) {
        TSDB_ERROR( p, "[TAOS][r=%d, %s]taos_query failed", r, taos_errstr(res) );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        taos_free_result( res );
        return r;
    }

    taos_free_result( res );
    return 0;
}

int wide_taos_conn_t::query_has_result( const char * sql, size_t sql_len, uint64_t & row_count )
{
    row_count = 0;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    TAOS_RES* res = taos_query( m_conn, sql );
    if ( NULL == res ) {
        TSDB_ERROR( p, "[TAOS]taos_query failed, SQL is coming..." );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return EFAULT;
    }

    int r = taos_errno( res );

    if ( 0 != r ) {
        TSDB_ERROR( p, "[TAOS][r=%d, %s]taos_query failed", r, taos_errstr(res) );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        taos_free_result( res );
        return r;
    }

    // travel each record and confirm all data has been sent to me and it already parsed.
    while ( true ) {
        TAOS_ROW row = taos_fetch_row( res );
        if ( NULL == row ) {
            break;
        }
        ++ row_count;
    }

    taos_free_result( res );
    return 0;
}

} // namespace wide
} // namespace test
} // namespace rtdb

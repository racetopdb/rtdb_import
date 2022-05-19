#include "wide_rtdb_conn.h"

namespace rtdb
{
namespace test
{
namespace wide
{

wide_rtdb_conn_t::wide_rtdb_conn_t()
{
}

wide_rtdb_conn_t::~wide_rtdb_conn_t()
{
}

void wide_rtdb_conn_t::kill_me()
{
    // do nothing, wide_rtdb_conn_t class only have one instance
}

int wide_rtdb_conn_t::select_db( const char * db )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r = p->select_db( p, db );
    if ( unlikely( 0 != r ) ) {
        TSDB_ERROR( p, "[RTDB][r=%d][db=%s] select_db failed", r, db );
        return r;
    }

    return 0;
}

int wide_rtdb_conn_t::query_non_result( const char * sql, size_t sql_len )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r = p->query( p, sql, (int)sql_len, NULL, NULL );
    if ( unlikely( 0 != r ) ) {
        TSDB_ERROR( p, "[RTDB][r=%d] query failed", r );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return r;
    }

    return 0;
}

int wide_rtdb_conn_t::query_has_result( const char * sql, size_t sql_len, uint64_t & row_count )
{
    row_count = 0;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int r = p->query( p, sql, (int)sql_len, NULL, NULL );
    if ( unlikely( 0 != r ) ) {
        TSDB_ERROR( p, "[RTDB][r=%d] query failed", r );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return r;
    }

    tsdb_v3_reader_t * reader = p->store_result( p );
    if ( unlikely( NULL == reader ) ) {
        TSDB_ERROR( p, "[RTDB] no recordset return" );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return ENODATA;
    }

    // travel each record and confirm all data has been sent to me and it already parsed.
    while ( true ) {
        r = reader->cursor_next( reader );
        if ( unlikely( 0 != r ) ) {
            break;
        }
        ++ row_count;
    }

    return 0;
}

} // namespace wide
} // namespace test
} // namespace rtdb

#include "wide_base.h"
#include <assert.h>
#include <string>

#define FIND_USE_TIMESTAMP    1

namespace rtdb
{
namespace test
{
namespace wide
{


// wide table, each table just only has one field.

struct thread_param_t
{
    thread_param_t()
        : lines()
    {}

    // Database Engine type
    db_type_t                   engine;

    wide_conn_t *             conn;

    // Thread index [0, thread_count)
    uint32_t                    thread_id;
    // Thread Count. by default, this value same with CPU core count.
    uint32_t                    thread_count;

    // first time by ms
    // in writing progress, the value updated to the time when the data is currently written.
    volatile uint64_t           start_time;
    // stop time
    uint64_t                    stop_time;
    // stop on local time
    uint64_t                    stop_local_time;
    // stop line in current thread
    uint64_t                    stop_line;
    // step time by ms
    uint32_t                    step_time;

    volatile uint64_t           find_count;
    // realtime find table count in this thread.
    volatile uint64_t           find_line_count;
    // We need find table count in this thread.
    uint32_t                    find_need;
    // find table from index
    uint32_t                    find_from;
    // error code, 0 indicate OK, error otherwise.
    int                         r;

    // point to line data array.
    std::vector< tsdb_str > *   lines;
    // database name
    const char *                db;

    // thread object
    pthread_t                   thread;
    // Is current thread already exited? 
    // current thread need set this value to true before quit the thread.
    volatile bool               exited;

    // padding memory, nothing.
    char                        padding[ 64 ];
};

void * find_table_thread( void * _param )
{
    thread_param_t * param = (thread_param_t *)_param;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    // calc the table count that We need to find. 
    param->find_need = (uint32_t)( param->lines->size() / (size_t)param->thread_count );
    // calc which index should I start with.
    param->find_from = param->find_need * param->thread_id;
    if ( param->thread_id == param->thread_count - 1 ) {
        // in last thread, we need add the remainder.
        param->find_need += (uint32_t)( param->lines->size() % (size_t)param->thread_count );
    }
    if ( 0 == param->find_from ) {
        // first line is DB name, we should ignore it.
        param->find_from = 1;
        -- param->find_need;
    }

    std::string name;
    std::string sql;

    try {
        sql.reserve( 256 );
    } catch ( ... ) {
        TSDB_ERROR( p, "[FIND][r=ENOMEM]" );
        param->r = ENOMEM;
        return NULL;
    }

    // USE DB
    param->r = param->conn->select_db( param->db );
    if ( unlikely( 0 != param->r ) ) {
        TSDB_ERROR( p, "[r=%d][db=%s] select_db failed", param->r, param->db );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size() );
        return NULL;
    }

    sql.resize( 0 );

    // find from each tables

    param->find_line_count = 0;

    tsdb_str buf[ 3 ];

    char time_b[ 64 ];
    char time_e[ 64 ];

    while ( true ) {

        // timestamp string
#if FIND_USE_TIMESTAMP
        sprintf( time_b, "%lld", (long long)param->start_time );

        sprintf( time_e, "%lld", (long long)(param->start_time + param->step_time - 1) );
#else
        int n;
        
        n = (int)sizeof(time_b) - 2;
        if ( unlikely( ! p->tools->datetime_to_str( param->start_time, & time_b[1], & n ) ) ) {
            TSDB_ERROR( p, "[FIND][thread_id=%d]datetime_to_str failed",
                        param->thread_id );
            break;
        }
        time_b[ 0 ]     = '"';
        time_b[ n + 1 ] = '"';
        time_b[ n + 2 ] = '\0';

        n = (int)sizeof(time_e) - 2;
        if ( unlikely( ! p->tools->datetime_to_str( param->start_time + param->step_time - 1, & time_e[1], & n ) ) ) {
            TSDB_ERROR( p, "[FIND][thread_id=%d]datetime_to_str failed",
                        param->thread_id );
            break;
        }
        time_e[ 0 ]     = '"';
        time_e[ n + 1 ] = '"';
        time_e[ n + 2 ] = '\0';

#endif // #if FIND_USE_TIMESTAMP #else

        // from every table
        for ( uint32_t table_index_in_thread = 0; table_index_in_thread < param->find_need; ++ table_index_in_thread ) {

            // calc the real index in global array.
            size_t i = table_index_in_thread + param->find_from;
            // get string item
            tsdb_str & item = param->lines->at( i );
            if ( unlikely( item.len <= 0 ) ) {
                continue;
            }

            // split the string to 3 row array:
            // 0   -    #TABLE
            // 1   -    table_name
            // 2   -    data type
            int buf_count = (int)COUNT_OF(buf);
            p->tools->to_const_array( item.ptr, item.len, ",", sizeof(",")-1, buf, & buf_count );
            if ( unlikely( COUNT_OF(buf) != buf_count ) ) {
                TSDB_ERROR( p, "[FIND][thread_id=%d][i=%d][count=%d, %d]count not match",
                            param->thread_id, (int)table_index_in_thread, (int)COUNT_OF(buf), (int)buf_count );
                continue;
            }
            if ( unlikely( buf[0].len != (int)sizeof("#TABLE")-1 || 0 != strncmp( "#TABLE", buf[0].ptr, buf[0].len ) ) ) {
                continue;
            }
            if ( unlikely( buf[1].len <= 0 || buf[2].len <= 0 ) ) {
                continue;
            }

            // construct the SQL statement.
            name.assign( buf[1].ptr, (size_t)buf[1].len );
            sql  = "select * from ";
            sql += name;
            sql += " where time between ";
            sql += time_b;
            sql += " and ";
            sql += time_e;
            sql += ";";

            // execute the SQL return value stored to param->r.
            uint64_t row_count;
            param->r = param->conn->query_has_result( sql.c_str(), sql.size(), row_count );
            ++ param->find_count;
            if ( unlikely( 0 != param->r ) ) {
                TSDB_ERROR( p, "[FIND][thread_id=%d][r=%d] find from table failed: %s", param->thread_id, param->r, sql.c_str() );
                break;
            }
            param->find_line_count += row_count;


            if ( unlikely( param->stop_line > 0 && param->find_line_count >= param->stop_line ) ) {
                TSDB_INFO( p, "[FIND][thread_id=%d]STOP", param->thread_id );
                break;
            }
        }

        if ( unlikely( param->stop_line > 0 && param->find_line_count >= param->stop_line ) ) {
            break;
        }

        param->start_time += param->step_time;
        if ( unlikely( param->stop_time > 0 && param->start_time >= param->stop_time ) ) {
            TSDB_INFO( p, "[FIND][thread_id=%d]STOP", param->thread_id );
            break;
        }

        if ( unlikely( param->stop_local_time > 0 && p->tools->datetime_now() >= param->stop_local_time ) ) {
            TSDB_INFO( p, "[FIND][thread_id=%d]STOP", param->thread_id );
            break;
        }
    }

    // quit the thread
    param->exited = true;
    return NULL;
}

int find_table( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    int r = 0;

    TSDB_INFO( p, "[FIND]FIND from Table" );

    // get db type: RTDB or TDEngine
    db_type_t engine = get_db_type( argc, argv );
    if ( DB_UNKNOWN == engine ) {
        TSDB_ERROR( p, "[FIND]get_db_type failed" );
        return r;
    }

    // -server 127.0.0.1:9000
    const char * server = NULL;
    p->tools->find_argv( argc, argv, "server", & server, NULL );
    if ( NULL == server || '\0' == * server ) {
        server = "127.0.0.1:9000";
    }
    TSDB_INFO( p, "[FIND][PARAMETERS][server      =%s]", server );


    wide_base_t * engine2 = wide_base_t::instance( engine, argc, argv );
    if ( NULL == engine2 ) {
        TSDB_ERROR( p, "[FIND][type=%d]create instance failed", (int)engine );
        return EFAULT;
    }

    // global connect server. RTDB supported just connect server one time.
    r = engine2->global_connect( server );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[FIND][CONNECT][server=%s]global_connect failed", server );
        return r;
    }

    // input file format.
    const char * format = NULL;
    p->tools->find_argv( argc, argv, "format", & format, NULL );
    if ( NULL == format || '\0' == * format ) {
        format = "txt";
    }
    TSDB_INFO( p, "[FIND][PARAMETERS][format      =%s]", format );

    // input file path
    std::string path;
    {
        const char * _path = NULL;
        p->tools->find_argv( argc, argv, "path", & _path, NULL );
        if ( NULL == _path || '\0' == * _path ) {
            if ( 0 == stricmp( "sql", format ) ) {
                _path = "./generate_table.sql";
            } else {
                _path = "./generate_table.txt";
            }
        }
        path = _path;
        p->tools->path_to_os( & path[0] );
    }
    TSDB_INFO( p, "[FIND][PARAMETERS][path        =%s]", path.c_str() );

    // find table thread count
    int thread_count;
    p->tools->find_argv_int( argc, argv, "thread", & thread_count );
    if ( thread_count <= 0 ) {
        thread_count = p->tools->get_cpu_core_count();
    }
    TSDB_INFO( p, "[FIND][PARAMETERS][thread      =%d]", thread_count );

    // find table start time
    uint64_t start_time = 0;
    p->tools->find_argv_datetime( argc, argv, "start_time", & start_time );
    if ( 0 == start_time ) {
        TSDB_ERROR( p, "[FIND][PARAMETERS][server=%s]invalid start_time", server );
        return EINVAL;
    }
    char start_time_s[ 64 ];
    int  start_time_sl = (int)sizeof(start_time_s);
    p->tools->datetime_to_str( start_time, start_time_s, & start_time_sl );
    TSDB_INFO( p, "[FIND][PARAMETERS][start_time  =%lld, %s]", start_time, start_time_s );

    // find table stop time
    uint64_t stop_time          = 0;
    uint64_t stop_local_time    = 0;
    do {
        const char * s = "";
        size_t       sl= 0;
        p->tools->find_argv( argc, argv, "stop_time", & s, & sl );
        if ( NULL == s || * s <= 0 || 0 == sl ) {
            break;
        }

        stop_time = p->tools->datetime_from_str( s, (int)sl );
        if ( stop_time > 0 ) {
            break;
        }

        if ( sl < 2 || ! isdigit( * s ) ) {
            TSDB_ERROR( p, "[FIND][PARAMETERS]invalid stop_time: %s", s );
            break;
        }

        int n = atoi( s );
        if ( n <= 0 ) {
            TSDB_ERROR( p, "[FIND][PARAMETERS]invalid stop_time: %s", s );
        }
        uint64_t now = p->tools->datetime_now();
        char c = s[ sl - 1 ];
        if ( 's' == c ) {
            stop_local_time = now + (uint64_t)1000 * (uint64_t)n;
        } else if ( 'm' == c ) {
            stop_local_time = now + (uint64_t)1000 * (uint64_t)60 * (uint64_t)n;
        } else if ( 'h' == c ) {
            stop_local_time = now + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)n;
        } else if ( 'd' == c ) {
            stop_local_time = now + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)24 * (uint64_t)n;
        } else if ( 'w' == c ) {
            stop_local_time = now + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)24 * (uint64_t)7 * (uint64_t)n;
        } else {
            TSDB_ERROR( p, "[FIND][PARAMETERS]invalid stop_time: %s", s );
        }
    } while ( 0 );
    if ( 0 != stop_time ) {
        char stop_time_s[ 64 ];
        int  stop_time_sl = (int)sizeof(stop_time_s);
        char start_time_s[ 64 ];
        int  start_time_sl = (int)sizeof(start_time_s);
        p->tools->datetime_to_str( stop_time, stop_time_s, & stop_time_sl );
        p->tools->datetime_to_str( start_time, start_time_s, & start_time_sl );
        if ( stop_time < start_time ) {
            TSDB_ERROR( p, "[FIND][PARAMETERS][stop_time=%lld, %s][start_time=%lld, %s]invalid time",
                           (long long)stop_time, stop_time_s, (long long)start_time, start_time_s );
            return EINVAL;
        }
        TSDB_INFO( p, "[FIND][PARAMETERS][stop_time   =%lld, %s]", (long long)stop_time, stop_time_s );
    } else {
        TSDB_INFO( p, "[FIND][PARAMETERS][stop_time   =NEVER]" );
    }
    if ( 0 != stop_local_time ) {
        char stop_time_s[ 64 ];
        int  stop_time_sl = (int)sizeof(stop_time_s);
        p->tools->datetime_to_str( stop_local_time, stop_time_s, & stop_time_sl );
        TSDB_INFO( p, "[FIND][PARAMETERS][stop_l_time =%lld, %s]", (long long)stop_local_time, stop_time_s );
    } else {
        TSDB_INFO( p, "[FIND][PARAMETERS][stop_l_time =NEVER]" );
    }

    // find table stop line
    int64_t stop_line = 0;
    uint64_t stop_line_per_thread;
    uint64_t stop_line_per_thread_m;
    p->tools->find_argv_int64( argc, argv, "stop_line", & stop_line );
    if ( stop_line > 0 ) {
        stop_line_per_thread    = (uint64_t)(stop_line / (int64_t)thread_count);
        stop_line_per_thread_m  = (uint64_t)(stop_line % (int64_t)thread_count);
        TSDB_INFO( p, "[FIND][PARAMETERS][stop_line   =%lld][stop_line_per_thread=%d]", (long long)stop_line, (long long)stop_line_per_thread );
    } else {
        TSDB_INFO( p, "[FIND][PARAMETERS][stop_line   =NEVER]" );
        stop_line               = 0;
        stop_line_per_thread    = 0;
        stop_line_per_thread_m  = 0;
    }


    // find table step_time
    int step_time;
    p->tools->find_argv_int( argc, argv, "step_time", & step_time );
    if ( step_time <= 0 ) {
        step_time = 3600000;
    }
    TSDB_INFO( p, "[FIND][PARAMETERS][step_time   =%d ms]", step_time );

    // get file size
    int64_t sz = p->tools->get_file_size( path.c_str() );
    if ( unlikely( sz <= 0 ) ) {
        TSDB_ERROR( p, "[FIND][path=%s]get_file_size failed", path.c_str() );
        return EINVAL;
    }

    TSDB_INFO( p, "[path=%s]Load the file content into memory", path.c_str() );
    std::string data;
    if ( sz > 0 ) {
        try {
            data.resize( (size_t)sz );
        } catch ( ... ) {
            TSDB_ERROR( p, "[FIND][path=%s][size=%lld]bad_alloc", path.c_str(), (long long)sz );
            return ENOMEM;
        }
        r = p->tools->load_file( path.c_str(), & data[0], & sz );
        if ( unlikely( 0 != r ) ) {
            TSDB_ERROR( p, "[FIND][path=%s][size=%lld][r=%d]load_file failed", path.c_str(), (long long)sz, r );
            return r;
        }
        data.resize( (size_t)sz );
    }
    if ( data.empty() ) {
        TSDB_ERROR( p, "[FIND][path=%s]file empty", path.c_str() );
        return EINVAL;
    }

    // We want split lines by \n. first step, we need to calculate the line count.
    std::vector< tsdb_str > lines;
    int count = 0;
    p->tools->to_const_array( data.c_str(), (int)data.size(), "\n", (int)sizeof("\n")-1, NULL, & count );
    if ( count <= 0 ) {
        TSDB_ERROR( p, "[FIND][path=%s]file empty", path.c_str() );
        return EINVAL;
    }
    // parpare the buffer cordinate the line count.
    try {
        lines.resize( count );
    } catch ( ... ) {
        TSDB_ERROR( p, "[FIND][lines=%d]No memory to load file", count );
        return ENOMEM;
    }
    // real split lines by \n
    p->tools->to_const_array( data.c_str(), (int)data.size(), "\n", (int)sizeof("\n")-1, & lines[0], & count );
    lines.resize( count );
    if ( count > 0 && 0 == lines[ count - 1 ].len ) {
        // delete last empty line
        -- count;
        lines.resize( count );
    }
    if ( count <= 1 ) {
        TSDB_ERROR( p, "[FIND][path=%s]file empty", path.c_str() );
        return EINVAL;
    }

    std::string db_name;
    std::string sql;


    // get Database name

    {
        // first line contains Database name:
        // #DB,database_name
    
        tsdb_str & item = lines[ 0 ];
        if (   item.len <= 0
            || item.len <= (int)sizeof(RTDB_wide_TXT_DB_LEAD)-1
        ) {
            TSDB_ERROR( p, "[FIND][line_count=%d][len=%d]first line invalid", count, item.len );
            return EINVAL;
        }
        // #DB,
        if ( 0 != strnicmp( RTDB_wide_TXT_DB_LEAD, item.ptr, sizeof(RTDB_wide_TXT_DB_LEAD)-1 ) ) {
            TSDB_ERROR( p, "[FIND][line_count=%d][len=%d]first line invalid", count, item.len );
            return EINVAL;
        }
        // store database name to db_name.
        const char * s = item.ptr + (sizeof(RTDB_wide_TXT_DB_LEAD)-1);
        const char * e = item.ptr + (size_t)item.len;
        db_name.assign( s, e );
        if ( db_name.empty() ) {
            TSDB_ERROR( p, "[FIND][line_count=%d][len=%d]db_name empty", count, item.len );
            return EINVAL;
        }

        /*
        // use the Database.
        sql = "use ";
        sql += db_name;
        TSDB_INFO( p, "%s", sql.c_str() );
        r = p->query( p, sql.c_str(), (int)sql.size(), NULL, NULL );
        if ( unlikely( 0 != r ) ) {
            TSDB_ERROR( p, "[FIND][db=%s][r=%d] use database failed", db_name.c_str(), r );
            return r;
        }
        */
    }

    // prepare FIND from TABLE threads

    std::vector< thread_param_t >   threads;
    threads.resize( thread_count );
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_t & item   = threads[ i ];
        item.thread_id          = (uint32_t)i;
        item.thread_count       = (uint32_t)threads.size();
        item.start_time         = start_time;
        item.stop_time          = stop_time;
        item.stop_local_time    = stop_local_time;
        item.stop_line          = stop_line_per_thread;
        if ( i == threads.size() - 1 ) {
            item.stop_line     += stop_line_per_thread_m;
        }
        item.step_time          = (uint32_t)step_time;
        item.find_count         = 0;
        item.find_line_count    = 0;
        item.find_need          = 0;
        item.find_from          = 0;
        item.engine             = engine;

        item.conn           = engine2->create_conn();
        if ( NULL == item.conn ) {
            TSDB_ERROR( p, "[CREATE][server=%s]create_conn failed", server );
            return EFAULT;
        }

        item.r                  = 0;
        item.lines              = & lines;
        item.db                 = db_name.c_str();
        item.exited             = true;
    }

    // start FIND from TABLE threads

    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_t & item = threads[ i ];
        item.exited = false;
        if ( unlikely( ! p->tools->thread_start( & item.thread, find_table_thread, & item, 0 ) ) ) {
            TSDB_ERROR( p, "[FIND]thread_start failed" );
            item.exited = true;
            return EFAULT;
        }
    }

    // waitting for FIND from TABLE threads stop

    unsigned long start             = p->tools->get_tick_count();
    unsigned long last_show         = start;
    uint64_t      last_count        = 0;
    uint64_t      last_find_count   = 0;

    while ( true ) {

        // sleep 100 ms
        p->tools->sleep_ms( 100 );

        unsigned long stop = p->tools->get_tick_count();
        unsigned long span = stop - last_show;
        if ( unlikely( span >= 1000 ) ) {

            // If last show find info time pass 1 s, then show find info now.

            // get current time as string, this for show to human.
            char s[ 64 ];
            int  sl = (int)sizeof(s);
            p->tools->datetime_to_str( p->tools->datetime_now(), s, & sl );

            // collect thread exit status, and find from table count.
            uint32_t exit_count     = 0;
            uint64_t count          = 0;
            uint64_t find_count     = 0;
            uint64_t curr_time_b    = 0;
            uint64_t curr_time_e    = 0;
            int      n;
            char     curr_time_bs[ 64 ];
            char     curr_time_es[ 64 ];
            for ( size_t i = 0; i < threads.size(); ++ i ) {
                thread_param_t & item = threads[ i ];
                count       += item.find_line_count;
                find_count  += item.find_count;
                if ( item.exited ) {
                    ++ exit_count;
                }
                if ( 0 == curr_time_b || 0 == curr_time_e ) {
                    curr_time_b = curr_time_e = item.start_time;
                } else {
                    if ( item.start_time < curr_time_b ) {
                        curr_time_b = item.start_time;
                    }
                    if ( item.start_time > curr_time_e ) {
                        curr_time_e = item.start_time;
                    }
                }
            }

            uint32_t speed = (uint32_t)(count - last_count);

            if ( curr_time_b > 0 ) {
                n = (int)sizeof(curr_time_bs);
                p->tools->datetime_to_str( curr_time_b, curr_time_bs, & n );
            } else {
                curr_time_bs[ 0 ] = '\0';
            }
            if ( curr_time_e > 0 ) {
                n = (int)sizeof(curr_time_es);
                p->tools->datetime_to_str( curr_time_e, curr_time_es, & n );
            } else {
                curr_time_es[ 0 ] = '\0';
            }

            if ( 0 == strcmp( curr_time_bs, curr_time_es ) ) {
                fprintf( stderr, "\r%s [FIND_TABLE][find.count=%lld][rows=%lld][table.count=%d][speed=%d/s][time=%s]...",
                        s, (long long)find_count, (long long)count, (int)(lines.size()-1), speed,
                        curr_time_bs
                );
            } else {
                fprintf( stderr, "\r%s [FIND_TABLE][find.count=%lld][rows=%lld][table.count=%d][speed=%d/s][time=%s -> %s]...",
                        s, (long long)find_count, (long long)count, (int)(lines.size()-1), speed,
                        curr_time_bs, curr_time_es
                );
            }

            // member this time
            last_show = stop;
            last_count = count;

            if ( exit_count == (uint32_t)threads.size() ) {
                // If all thread exited, then exit the loop now.
                break;
            }
        }
    }
    fprintf( stderr, "\n" );
    
    uint64_t find_count = 0;
    uint64_t line_count = 0;
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_t & item = threads[ i ];
        line_count += item.find_line_count;
        find_count += item.find_count;
    }

    unsigned long stop = p->tools->get_tick_count();
    unsigned long span = stop - start;
    TSDB_INFO( p, "[FIND_TABLE][table.count=%d][find.count=%lld][rows=%lld][use=%d ms]",
                  (int)(lines.size()-1), (long long)find_count, (long long)line_count, span );

    // Close Thread
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_t & item = threads[ i ];
        p->tools->thread_join( & item.thread );

        if ( item.conn ) {
            item.conn->kill_me();
            item.conn = NULL;
        }
    }

    return r;
}

} // namespace wide
} // namespace test
} // namespace rtdb

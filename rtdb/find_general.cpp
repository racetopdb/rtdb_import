#include "wide_base.h"
#include <assert.h>
#include <string>
#include <map>
#include "utils.h"

#define FIND_USE_TIMESTAMP    1

namespace rtdb
{

namespace wide
{


// wide table, each table just only has one field.

struct thread_param_find_table_general_t
{
    thread_param_find_table_general_t()
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

    // 表名前缀和表详细信息映射关系  
    std::map<std::string, struct test_table_file_info_t> *map_test_table_file_info_t;

    // 表名数组  每个线程获取自己的数据下标即可    
    std::vector<std::vector<struct table_lead_and_table_name_t> > *vt_vt_table_lead_and_table_name_t;

    // 强制退出标志  默认false
    volatile bool force_to_exit;

    // 找到点数  
    volatile uint64_t           find_point;

    // padding memory, nothing.
    char                        padding[ 64 ];
};

void * find_table_general_thread( void * _param )
{
    thread_param_find_table_general_t * param = (thread_param_find_table_general_t *)_param;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );



    std::string name;
    std::string sql;

    try {
        sql.reserve( 256 );
    } catch ( ... ) {
        TSDB_ERROR( p, "[FIND][r=ENOMEM]" );
        param->r = ENOMEM;
        return NULL;
    }

    sql.resize( 0 );

    // USE DB
    param->r = param->conn->select_db( param->db );
    if ( unlikely( 0 != param->r ) ) {
        TSDB_ERROR( p, "[r=%d][db=%s] select_db failed", param->r, param->db );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size() );
        return NULL;
    }

    // find from each tables
   
    param->find_line_count = 0;


    char time_b[64];
    char time_e[64];
    int  time_b_sl = 0;
    int  time_e_sl = 0;

    while ( !param->force_to_exit ) {

        // timestamp string
#if FIND_USE_TIMESTAMP
        //sprintf( time_b, "%lld", (long long)param->start_time );
        //sprintf( time_e, "%lld", (long long)(param->start_time + param->step_time - 1) );
        
        memset(time_b, 0x00, sizeof(time_b));
        time_b_sl = (int)sizeof(time_b);
        p->tools->datetime_to_str(param->start_time, time_b, &time_b_sl);

        memset(time_e, 0x00, sizeof(time_e));
        time_e_sl = (int)sizeof(time_e);
        p->tools->datetime_to_str(param->start_time + param->step_time - 1, time_e, &time_e_sl);
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

        std::vector<struct table_lead_and_table_name_t> &vt_table_lead_and_table_name_t = 
            (*param->vt_vt_table_lead_and_table_name_t)[param->thread_id];

        // 一个表都没有 则直接退出吧   
        if (vt_table_lead_and_table_name_t.empty()) {
            break;
        }

        // from every table
        for ( size_t table_index_in_thread = 0; table_index_in_thread < vt_table_lead_and_table_name_t.size(); ++table_index_in_thread ) {

            // calc the real index in global array.
            size_t i = table_index_in_thread;
            
            struct table_lead_and_table_name_t &tlatn = vt_table_lead_and_table_name_t[i];
            struct test_table_file_info_t & ttfi  = (*param->map_test_table_file_info_t)[tlatn.table_lead];

            // construct the SQL statement.
            name.assign(tlatn.table_name);
            sql  = "select * from ";
            sql += name;
            sql += " where time between ";
            sql += "\'";
            sql += time_b;
            sql += "\'";
            sql += " and ";
            sql += "\'";
            sql += time_e;
            sql += "\'";
            sql += ";";

            // execute the SQL return value stored to param->r.
            uint64_t row_count;
            param->r = param->conn->query_has_result( sql.c_str(), sql.size(), row_count );
            if ( unlikely( 0 != param->r ) ) {
                TSDB_ERROR( p, "[FIND][thread_id=%d][r=%d] find from table failed: %s", param->thread_id, param->r, sql.c_str() );
                param->exited = true;
                return NULL;
            }
            ++ param->find_count;
            param->find_line_count += row_count;
            param->find_point += (row_count*(ttfi.vt_test_tb_field_info_t.size()-1));  // 去除主键  

            if ( unlikely( param->stop_line > 0 && param->find_line_count >= param->stop_line ) ) {
                TSDB_INFO( p, "[FIND][thread_id=%d]STOP", param->thread_id );
                param->exited = true;
                return NULL;
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
    }

    // quit the thread
    param->exited = true;
    return NULL;
}

int find_table_general( int argc, char ** argv )
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
            if (NULL == _path || '\0' == *_path) {
                TSDB_ERROR(p, "[INSERT] not found table_conf");
                return EINVAL;
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
    uint64_t stop_time = 0;
    do {
        const char* s = "";
        size_t       sl = 0;
        p->tools->find_argv(argc, argv, "stop_time", &s, &sl);
        if (NULL == s || *s <= 0 || 0 == sl) {
            break;
        }

        stop_time = p->tools->datetime_from_str(s, (int)sl);
        if (stop_time > 0) {
            break;
        }

        if (sl < 2 || !isdigit(*s)) {
            TSDB_ERROR(p, "[FIND][PARAMETERS]invalid stop_time: %s", s);
            break;
        }

        int n = atoi(s);
        if (n <= 0) {
            TSDB_ERROR(p, "[FIND][PARAMETERS]invalid stop_time: %s", s);
        }
        //uint64_t now = p->tools->datetime_now();
        char c = s[sl - 1];
        if ('s' == c) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)n;
        }
        else if ('m' == c) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)n;
        }
        else if ('h' == c) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)n;
        }
        else if ('d' == c) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)24 * (uint64_t)n;
        }
        else if ('w' == c) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)24 * (uint64_t)7 * (uint64_t)n;
        }
        else {
            TSDB_ERROR(p, "[FIND][PARAMETERS]invalid stop_time: %s", s);
        }
    } while (0);
    if (0 != stop_time) {
        char stop_time_s[64];
        int  stop_time_sl = (int)sizeof(stop_time_s);
        char start_time_s[64];
        int  start_time_sl = (int)sizeof(start_time_s);
        p->tools->datetime_to_str(stop_time, stop_time_s, &stop_time_sl);
        p->tools->datetime_to_str(start_time, start_time_s, &start_time_sl);
        if (stop_time < start_time) {
            TSDB_ERROR(p, "[FIND][PARAMETERS][stop_time=%lld, %s][start_time=%lld, %s]invalid time",
                (long long)stop_time, stop_time_s, (long long)start_time, start_time_s);
            return EINVAL;
        }
        TSDB_INFO(p, "[FIND][PARAMETERS][stop_time   =%lld, %s]", (long long)stop_time, stop_time_s);
    }
    else {
        TSDB_INFO(p, "[FIND][PARAMETERS][stop_time   =NEVER]");
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


    std::string db_name;
    std::string sql;
    // get Database name
    {
        // first line contains Database name:
        // -db DB_TEST_WRITE
        const char* db = NULL;
        p->tools->find_argv(argc, argv, "db", &db, NULL);
        if (NULL == db || '\0' == *db) {
            db = "DB_TEST_WRITE";
        }
        TSDB_INFO(p, "[FIND][PARAMETERS][db      =%s]", db);


        db_name = db;
        if (db_name.empty()) {
            TSDB_ERROR(p, "[FIND]db_name empty");
            return EINVAL;
        }
    }


    // 解析表配置文件中的内容  
    std::map<std::string, struct test_table_file_info_t> map_test_table_file_info_t;
    r = parse_table_conf_file(path.c_str(), map_test_table_file_info_t);
    if (0 != r) {
        TSDB_INFO(p, "[CREATE][table_conf=%s] parse_table_conf_file failed", path.c_str());
        return r;
    }

    std::vector<struct table_lead_and_table_name_t> vt_table_lead_and_table_name_t;

    // 将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
    r = convert_table_conf_map_to_table_vector(map_test_table_file_info_t, vt_table_lead_and_table_name_t);
    if (0 != r) {
        TSDB_INFO(p, "[CREATE][path=%s] convert_table_conf_map_to_table_vector failed", path.c_str());
        return r;
    }

    std::vector<std::vector<struct table_lead_and_table_name_t> > vt_vt_table_lead_and_table_name_t;
    vt_vt_table_lead_and_table_name_t.resize(thread_count);
    size_t i = 0;
    for (; i < vt_table_lead_and_table_name_t.size(); )
    {
        for (int j = 0; j < thread_count && i < vt_table_lead_and_table_name_t.size(); j++, i++)
        {
            struct table_lead_and_table_name_t& tlatn = vt_table_lead_and_table_name_t[i];
            vt_vt_table_lead_and_table_name_t[j].push_back(tlatn);
        }
    }
    
    for (size_t i = 0; i < vt_vt_table_lead_and_table_name_t.size(); i++)
    {
        TSDB_INFO(p, "");
        for (size_t j=0; j<vt_vt_table_lead_and_table_name_t[i].size(); j++)
        {
            struct table_lead_and_table_name_t& tlatn = vt_vt_table_lead_and_table_name_t[i][j];
            TSDB_INFO(p, "[thread_id:%d][index:%d] table_name:%s",(int)i,(int)j, tlatn.table_name.c_str());
        }
    }
    TSDB_INFO(p, "");


    // prepare FIND from TABLE threads
    std::vector< thread_param_find_table_general_t >   threads;
    threads.resize( thread_count );
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_find_table_general_t & item   = threads[ i ];
        item.engine             = engine;
        item.conn = engine2->create_conn();
        if (NULL == item.conn) {
            TSDB_ERROR(p, "[FIND][server=%s]create_conn failed", server);
            return EFAULT;
        }
        item.thread_id          = (uint32_t)i;
        item.thread_count       = (uint32_t)threads.size();
        item.start_time         = start_time;
        item.stop_time          = stop_time;
        item.stop_local_time    = 0;
        item.stop_line          = stop_line_per_thread;
        if ( i == threads.size() - 1 ) {
            item.stop_line     += stop_line_per_thread_m;
        }
        item.step_time          = (uint32_t)step_time;
        item.find_count         = 0;
        item.find_line_count    = 0;
        item.find_need          = 0;
        item.find_from          = 0;
        item.r                  = 0;
        item.lines              = NULL;
        item.db                 = db_name.c_str();
        item.thread             = 0;
        item.exited             = false;
        item.map_test_table_file_info_t = &map_test_table_file_info_t;
        item.vt_vt_table_lead_and_table_name_t = &vt_vt_table_lead_and_table_name_t;
        item.force_to_exit = false;
        item.find_point = 0;
    }

    // start FIND from TABLE threads
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_find_table_general_t & item = threads[ i ];
        item.exited = false;
        if ( unlikely( ! p->tools->thread_start( & item.thread, find_table_general_thread, & item, 0 ) ) ) {
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
    uint64_t      last_find_point_count   = 0;

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
            uint64_t find_point_count     = 0;
            uint64_t curr_time_b    = 0;
            uint64_t curr_time_e    = 0;
            int      n;
            char     curr_time_bs[ 64 ];
            char     curr_time_es[ 64 ];
            for ( size_t i = 0; i < threads.size(); ++ i ) {
                thread_param_find_table_general_t & item = threads[ i ];
                count       += item.find_line_count;
                find_count  += item.find_count;
                find_point_count  += item.find_point;
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
            uint32_t point_speed = (uint32_t)(find_point_count - last_find_point_count);

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

            if (0 == strcmp(curr_time_bs, curr_time_es)) {
                fprintf(stderr, "\r%s [FIND_TABLE][TABLE COUNT=%d][QUERY COUNT=%lld][ROWS=%lld][SPEED=%d (ROWS/S), %d (POINTS/S)][time=%s]...",
                    s, (int)(vt_table_lead_and_table_name_t.size()),
                    (long long)find_count, (long long)count,
                    speed, point_speed, curr_time_bs
                );
            }
            else {
                fprintf(stderr, "\r%s [FIND_TABLE][TABLE COUNT=%d][QUERY COUNT=%lld][ROWS=%lld][SPEED=%d (ROWS/S), %d (POINTS/S)][time=%s -> %s]...",
                    s, (int)(vt_table_lead_and_table_name_t.size()),
                    (long long)find_count, (long long)count,
                    speed, point_speed, curr_time_bs, curr_time_es
                );
            }

            // member this time
            last_show = stop;
            last_count = count;
            last_find_point_count = find_point_count;

            if ( exit_count == (uint32_t)threads.size() ) {
                // If all thread exited, then exit the loop now.
                break;
            }

            // 停止行数不为0 且当前行数已经大于或者等于停止行数了  则直接发送退出指令  
            if (stop_line > 0 && count >= (uint64_t)stop_line) {
                
                for ( size_t i = 0; i < threads.size(); ++ i )
                {
                    thread_param_find_table_general_t & item = threads[ i ];
                    item.force_to_exit = true;
                }
            }

        }
    }
    fprintf( stderr, "\n" );
    
    uint64_t find_count = 0;
    uint64_t line_count = 0;
    uint64_t find_point_count = 0;
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_find_table_general_t & item = threads[ i ];
        line_count += item.find_line_count;
        find_count += item.find_count;
        find_point_count += item.find_point;

    }

    unsigned long stop = p->tools->get_tick_count();
    unsigned long span = stop - start;
    TSDB_INFO(p, "[FIND_TABLE][TABLE COUNT=%d][QUERY COUNT=%lld][ROWS=%lld][POINTS=%lld][use=%d ms]",
        (int)(vt_table_lead_and_table_name_t.size()),
        (long long)find_count, (long long)line_count,
        (long long)find_point_count, span);

    // Close Thread
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_find_table_general_t & item = threads[ i ];
        p->tools->thread_join( & item.thread );

        if ( item.conn ) {
            item.conn->kill_me();
            item.conn = NULL;
        }
    }
    print_current_path();
    return r;
}

} // namespace wide

} // namespace rtdb

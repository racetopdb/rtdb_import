#include "wide_base.h"
#include "utils.h"
#include <assert.h>
#include <string>
#include <sstream>

//此文件是基于create_table.cpp 修改的 数据库名字是基于外部参数传递进来的, 内部文件中的数据库则直接忽略了 -db DB_TEST_WRITE  

namespace rtdb
{

namespace wide
{

struct thread_param_create_table_general_t
{
    // Database Engine type
    db_type_t                   engine;

    wide_conn_t *             conn;

    // Thread index [0, thread_count)
    uint32_t                    thread_id;
    // Thread Count. by default, this value same with CPU core count.
    uint32_t                    thread_count;

    // realtime created table count in this thread.
    volatile uint32_t           create_count;
    // We need create table count in this thread.
    uint32_t                    create_need;
    // Create table from index
    uint32_t                    create_from;
    // error code, 0 indicate OK, error otherwise.
    int                         r;

    // point to line data array. 这个成员废弃 什么用  
    std::vector< tsdb_str > *   lines;
    // database name
    const char *                db;

    // thread object
    pthread_t                   thread;
    // Is current thread already exited? 
    // current thread need set this value to true before quit the thread.
    volatile bool               exited;

    // 表配置信息  
    std::map<std::string, struct test_table_file_info_t>* map_test_table_file_info_t;

    // 表名前缀和表名  
    std::vector<struct table_lead_and_table_name_t>* vt_table_lead_and_table_name_t;

    // padding memory, nothing.
    char                        padding[ 64 ];
};





// 创建表的sql语句目前仅处理rtdb  
static int build_create_table_sql_for_rtdb(
    const std::string& table_name,
    const std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t,
    std::string &sql)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    sql.resize(0);

    std::stringstream ss;
    ss << "create table ";
    ss << " if not exists ";
    ss << table_name;
    ss << " (";

    //NOTE: 目前服务器端如果加入主键会报错 此处 先忽略   
    for (size_t i = 1; i < vt_test_tb_field_info_t.size(); i++)
    {
        if (1 != i) {
            ss << ",";
            ss << " ";
        }

        if (unlikely(TSDB_DATATYPE_STRING == vt_test_tb_field_info_t[i].datatype)) {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << vt_test_tb_field_info_t[i].type;
            ss << "(";
            ss << vt_test_tb_field_info_t[i].len;
            ss << ")";
            // 增加字段是否为空sql 语句   
            if (likely(vt_test_tb_field_info_t[i].is_null)) {
                ss << " ";
                ss << "NULL";
            }
            else {
                ss << " ";
                ss << "NOT  NULL";
            }
        }
        else {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << vt_test_tb_field_info_t[i].type;
            // 增加字段是否为空sql 语句  
            if (likely(vt_test_tb_field_info_t[i].is_null)) {
                ss << " ";
                ss << "NULL";
            }
            else {
                ss << " ";
                ss << "NOT  NULL";
            }
        }
    }

    ss << ")";
    ss << ";";

    sql = ss.str();

    return r;
}


// 创建表的sql语句目前仅处理taos  
static int build_create_table_sql_for_taos(
    const std::string& table_name,
    const std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t,
    std::string &sql)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    sql.resize(0);

    std::stringstream ss;
    ss << "create table ";
    ss << " if not exists ";
    ss << table_name;
    ss << " (";

    //加入主键 默认名字为'time'   
    // NOTE: 注意taos 除了主键外其他字段默认可以为NULL
    for (size_t i = 0; i < vt_test_tb_field_info_t.size(); i++)
    {
        if (0 != i) {
            ss << ",";
            ss << " ";
        }

        if (unlikely(TSDB_DATATYPE_STRING == vt_test_tb_field_info_t[i].datatype)) {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << "binary";
            ss << "(";
            ss << vt_test_tb_field_info_t[i].len;
            ss << ")";
            
        }
        else {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << vt_test_tb_field_info_t[i].type;
        }
    }

    ss << ")";
    ss << ";";

    sql = ss.str();

    return r;
}


// 创建表的sql语句目前仅处理timescaledb  
static int build_create_table_sql_for_timescaledb(
    const std::string& table_name,
    const std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t,
    std::string& sql)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    sql.resize(0);

    std::stringstream ss;
    ss << "create table ";
    ss << " if not exists ";
    ss << table_name;
    ss << " (";

    //NOTE: 目前服务器端必须加入主键默认设置time好了 其实可以随便设置的    
    for (size_t i = 0; i < vt_test_tb_field_info_t.size(); i++)
    {
        if (0 != i) {
            ss << ",";
            ss << " ";
        }

        // 主键  
        if (0 == i) {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << vt_test_tb_field_info_t[i].type;
            ss << " ";
            ss << "PRIMARY KEY";
            continue;
        }

        if (unlikely(TSDB_DATATYPE_STRING == vt_test_tb_field_info_t[i].datatype)) {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << vt_test_tb_field_info_t[i].type;
            ss << "(";
            ss << vt_test_tb_field_info_t[i].len;
            ss << ")";
            // 增加字段是否为空sql 语句   
            if (likely(vt_test_tb_field_info_t[i].is_null)) {
                ss << " ";
                ss << "NULL";
            }
            else {
                ss << " ";
                ss << "NOT  NULL";
            }
        }
        else {
            ss << vt_test_tb_field_info_t[i].name;
            ss << " ";
            ss << vt_test_tb_field_info_t[i].type;
            // 增加字段是否为空sql 语句  
            if (likely(vt_test_tb_field_info_t[i].is_null)) {
                ss << " ";
                ss << "NULL";
            }
            else {
                ss << " ";
                ss << "NOT  NULL";
            }
        }
    }

    ss << ")";
    ss << ";";

    sql = ss.str();

    return r;
}

// 创建表的sql语句目前仅处理rtdb taos  
static int build_create_table_sql(
    db_type_t engine,
    const std::string& table_name,
    const std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t,
    std::string &sql)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    switch (engine)
    {
    case rtdb::wide::DB_RTDB:
    {
        return build_create_table_sql_for_rtdb(table_name, vt_test_tb_field_info_t, sql);
    }
    case rtdb::wide::DB_TAOS:
    {
        return build_create_table_sql_for_taos(table_name, vt_test_tb_field_info_t, sql);
    }
    case rtdb::wide::DB_TIMESCALEDB:
    {
        return build_create_table_sql_for_timescaledb(table_name, vt_test_tb_field_info_t, sql);
    }
    case rtdb::wide::DB_UNKNOWN:
    default:

        TSDB_ERROR( p, "[r=%d][db_type:%d] invalid db_type", (int)engine);
        return EINVAL;
    }

    return r;
}



void * create_table_general_thread( void * _param )
{
    thread_param_create_table_general_t * param = (thread_param_create_table_general_t *)_param;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    // calc the table count that We need to create. 
    param->create_need = (uint32_t)( param->vt_table_lead_and_table_name_t->size() / (size_t)param->thread_count );
    // calc which index should I start with.
    param->create_from = param->create_need * param->thread_id;
    if ( param->thread_id == param->thread_count - 1 ) {
        // in last thread, we need add the remainder.
        param->create_need += (uint32_t)( param->vt_table_lead_and_table_name_t->size() % (size_t)param->thread_count );
    }
    

    std::string sql;
    sql.reserve(8192);
    sql.resize(0);

    // USE DB
    param->r = param->conn->select_db( param->db );
    if ( unlikely( 0 != param->r ) ) {
        TSDB_ERROR( p, "[r=%d][db=%s] select_db failed", param->r, param->db );
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size() );
        return NULL;
    }

    // create each tables
    for ( param->create_count = 0; param->create_count < param->create_need; ++ param->create_count ) {

        // calc the real index in global array.
        size_t i = param->create_count + param->create_from;
        
        //获取表名前缀和表名  
        struct table_lead_and_table_name_t &tlatn = (*(param->vt_table_lead_and_table_name_t))[i];

        std::map<std::string, struct test_table_file_info_t>::iterator iter = 
            param->map_test_table_file_info_t->find(tlatn.table_lead);
        if (iter == param->map_test_table_file_info_t->end()) {
            param->exited = true;
            param->r = ENOENT;
            TSDB_ERROR(p, "[CREATE][table_lead:%s] not found", tlatn.table_lead.c_str());
            return NULL;
        }

        std::vector<struct test_tb_field_info_t> & vt_test_tb_field_info_t = iter->second.vt_test_tb_field_info_t;

        // 构建sql 语句 目前仅仅支持  rtdb, taos   
        param->r = build_create_table_sql(param->engine, tlatn.table_name, vt_test_tb_field_info_t, sql);
        if (unlikely(0 != param->r)) {
            TSDB_ERROR(p, "[CREATE][thread_id=%d][table_name:%s][r=%d] build_create_table_sql failed",
                param->thread_id, tlatn.table_name.c_str(), param->r);
            break;
        }

        // execute the SQL return value stored to param->r.
        param->r = param->conn->query_non_result( sql.c_str(), sql.size() );
        if ( unlikely( 0 != param->r ) ) {
            TSDB_ERROR( p, "[CREATE][thread_id=%d][table_name:%s][r=%d] create table failed", 
                param->thread_id, tlatn.table_name.c_str() , param->r );
            p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size() );
            break;
        }
    }

    // quit the thread
    param->exited = true;
    return NULL;
}

int create_table_general( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    int r = 0;

    TSDB_INFO( p, "[CREATE]Create Table" );

    // get db type: RTDB or TDEngine
    db_type_t engine = get_db_type( argc, argv );
    if ( DB_UNKNOWN == engine ) {
        TSDB_ERROR( p, "[CREATE]get_db_type failed" );
        return r;
    }

    // -server 127.0.0.1:9000
    const char * server = NULL;
    p->tools->find_argv( argc, argv, "server", & server, NULL );
    if ( NULL == server || '\0' == * server ) {
        server = "127.0.0.1:9000";
    }
    TSDB_INFO( p, "[CREATE][PARAMETERS][server      =%s]", server );

    wide_base_t * engine2 = wide_base_t::instance( engine, argc, argv );
    if ( NULL == engine2 ) {
        TSDB_ERROR( p, "[CREATE][type=%d]create instance failed", (int)engine );
        return EFAULT;
    }

    // global connect server. RTDB supported just connect server one time.
    r = engine2->global_connect( server );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[CREATE][CONNECT][server=%s]global_connect failed", server );
        return r;
    }

    // input file format.
    const char * format = NULL;
    p->tools->find_argv( argc, argv, "format", & format, NULL );
    if ( NULL == format || '\0' == * format ) {
        format = "txt";
    }
    TSDB_INFO( p, "[CREATE][PARAMETERS][format      =%s]", format );

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
    TSDB_INFO( p, "[CREATE][PARAMETERS][path        =%s]", path.c_str() );

    // create table thread count
    int thread_count;
    p->tools->find_argv_int( argc, argv, "thread", & thread_count );
    if ( thread_count <= 0 ) {
        thread_count = p->tools->get_cpu_core_count();
    }
    TSDB_INFO( p, "[CREATE][PARAMETERS][thread      =%d]", thread_count );



    // 解析文件中的内容  
    std::map<std::string, struct test_table_file_info_t> map_test_table_file_info_t;
    r = parse_table_conf_file(path.c_str(), map_test_table_file_info_t);
    if (0 != r) {
        TSDB_INFO( p, "[CREATE][path=%s] parse_table_conf_file failed", path.c_str() );
        return r;
    }

    std::vector<struct table_lead_and_table_name_t> vt_table_lead_and_table_name_t;

    // 将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
    r = convert_table_conf_map_to_table_vector(map_test_table_file_info_t, vt_table_lead_and_table_name_t);
    if (0 != r) {
        TSDB_INFO(p, "[CREATE][path=%s] convert_table_conf_map_to_table_vector failed", path.c_str());
        return r;
    }


    std::string db_name;
    std::string sql;

    // Create Database

    {
   
        // first line contains Database name:

        // -db DB_TEST_WRITE
        const char* db = NULL;
        p->tools->find_argv(argc, argv, "db", &db, NULL);
        if (NULL == db || '\0' == *db) {
            db = "DB_TEST_WRITE";
        }
        TSDB_INFO(p, "[CREATE][PARAMETERS][db      =%s]", db);


        db_name = db;
        if ( db_name.empty() ) {
            TSDB_ERROR( p, "[CREATE]db_name empty");
            return EINVAL;
        }

        wide_conn_t* conn = engine2->create_conn();
        if (unlikely(NULL == conn)) {
            TSDB_ERROR(p, "[CREATE][db=%s] create_conn failed", db_name.c_str());
            return EFAULT;
        }

        if (DB_TIMESCALEDB == engine) {
            // 如果是timescaledb的话先删除数据库再创建数据库 解决数据库存在再创建报错的情况！！！  
            // 这个也是没办法的办法 看看能不能搞定  
            sql = "DROP DATABASE IF EXISTS ";
            sql += db_name;
            sql += ";";
            // execute SQL to create database
            r = conn->query_non_result(sql.c_str(), (int)sql.size());
            if (unlikely(0 != r)) {
                TSDB_ERROR(p, "[CREATE][db=%s][r=%d] create database failed", db_name.c_str(), r);
                p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size());
                conn->kill_me();
                return r;
            }
        }

        // construct the SQL statement
        // construct the SQL statement
        sql = "create database ";
        // timescaledb 不支持  
        if (DB_TIMESCALEDB != engine) {
            sql += " if not exists ";
        }
        sql += db_name;

        if (DB_RTDB == engine) {
            // setup this database has 40 concurrent write
            sql += " with concurrent=40";
        }
        sql += ";";

        // execute SQL to create database
        r = conn->query_non_result( sql.c_str(), (int)sql.size() );
        if ( unlikely( 0 != r ) ) {
            TSDB_ERROR( p, "[CREATE][db=%s][r=%d] create database failed", db_name.c_str(), r );
            p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size() );
            conn->kill_me();
            return r;
        }

        conn->kill_me();
        conn = NULL;
    }

    // prepare CREATE TABLE threads

    std::vector< thread_param_create_table_general_t >   threads;
    threads.resize( thread_count );
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_create_table_general_t & item = threads[ i ];
        item.engine = engine;
        item.conn = engine2->create_conn();
        if (NULL == item.conn) {
            TSDB_ERROR(p, "[CREATE][server=%s]create_conn failed", server);
            return EFAULT;
        }
        item.thread_id = (uint32_t)i;
        item.thread_count = (uint32_t)threads.size();
        item.create_count = 0;
        item.create_need = 0;
        item.create_from = 0;
        item.r = 0;
        item.lines = NULL;
        item.db = db_name.c_str();
        item.thread = 0;
        item.exited = false;
        item.map_test_table_file_info_t = &map_test_table_file_info_t;
        item.vt_table_lead_and_table_name_t = &vt_table_lead_and_table_name_t;
    }

    // start CREATE TABLE threads

    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_create_table_general_t & item = threads[ i ];
        item.exited = false;
        if ( unlikely( ! p->tools->thread_start( & item.thread, create_table_general_thread, & item, 0 ) ) ) {
            TSDB_ERROR( p, "[CREATE]thread_start failed" );
            item.exited = true;
            return EFAULT;
        }
    }

    // waitting for CREATE TABLE threads stop

    unsigned long start         = p->tools->get_tick_count();
    unsigned long last_show     = start;
    uint32_t      last_count    = 0;

    while ( true ) {

        // sleep 100 ms
        p->tools->sleep_ms( 100 );

        unsigned long stop = p->tools->get_tick_count();
        unsigned long span = stop - last_show;
        if ( unlikely( span >= 1000 ) ) {

            // If last show create info time pass 1 s, then show create info now.

            // get current time as string, this for show to human.
            char s[ 64 ];
            int  sl = (int)sizeof(s);
            p->tools->datetime_to_str( p->tools->datetime_now(), s, & sl );

            // collect thread exit status, and create table count.
            uint32_t exit_count = 0;
            uint32_t count = 0;
            for ( size_t i = 0; i < threads.size(); ++ i ) {
                thread_param_create_table_general_t & item = threads[ i ];
                count += item.create_count;
                if ( item.exited ) {
                    ++ exit_count;
                }
            }

            uint32_t speed = count - last_count;
            uint32_t rest;
            if ( speed > 0 ) {
                // calculate the speed, be careful for div 0.
                rest = (uint32_t)(vt_table_lead_and_table_name_t.size() / speed);
            } else {
                rest = 0;
            }

            fprintf( stderr, "\r%s [CREATE_TABLE][%d / %d][speed=%d/s][rest=%d s]...",
                    s, (int)count, (int)vt_table_lead_and_table_name_t.size(), speed, rest
            );

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
    
    unsigned long stop = p->tools->get_tick_count();
    unsigned long span = stop - start;
    TSDB_INFO( p, "[CREATE_TABLE][table_count=%d][use=%d ms]", (int)vt_table_lead_and_table_name_t.size(), span );

    // Close Thread
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_create_table_general_t & item = threads[ i ];
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

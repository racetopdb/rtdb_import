#include "wide_timescaledb_conn.h"



#define TIMESCALEDB_USER   "postgres"
#define TIMESCALEDB_PASSWD "postgres"

namespace rtdb
{
namespace test
{
namespace wide
{

// 是否启用 s_m_is_timescaledb_enable 默认 false  
volatile bool wide_timescaledb_conn_t::s_m_is_timescaledb_enable = false;

// 锁  
struct lockable_t wide_timescaledb_conn_t::s_m_lockable;

wide_timescaledb_conn_t::wide_timescaledb_conn_t()
    : m_conn( NULL ), m_conninfo(),m_qureysql()
{
    m_qureysql.reserve(256 * 1024); // 256K 
    m_qureysql.resize(0);
}

wide_timescaledb_conn_t::~wide_timescaledb_conn_t()
{
    disconnect();
    m_conninfo.resize(0);
    m_qureysql.resize(0);
}

void wide_timescaledb_conn_t::kill_me()
{
    delete this;
}

int wide_timescaledb_conn_t::connect( const char * server )
{
    tsdb_v3_t * p = rtdb_tls();
    assert( p );


    //PGconn *conn = PQconnectdb("hostaddr=192.168.1.40 port=5432 user=postgres password=postgres dbname=mydb4");
    int count = 0;
    p->tools->to_const_array(server, (int)strlen(server), ":", 1, NULL, &count);
    if (2 != count) {
        TSDB_ERROR(p, "[TIMESCALEDB][server=%s]invalid server string", server);
        return EINVAL;
    }

    std::vector<tsdb_str> vt_tsdb_str;
    vt_tsdb_str.resize(count);
    p->tools->to_const_array(server, (int)strlen(server), ":", 1, &vt_tsdb_str[0], &count);
    if (2 != count) {
        TSDB_ERROR(p, "[TIMESCALEDB][server=%s]invalid server string", server);
        return EINVAL;
    }

    
    m_conninfo.reserve(256);
    m_conninfo.resize(0);
    m_conninfo = "hostaddr=";
    m_conninfo += std::string(vt_tsdb_str[0].ptr, vt_tsdb_str[0].len);
    m_conninfo += " ";

    m_conninfo += "port=";
    m_conninfo += std::string(vt_tsdb_str[1].ptr, vt_tsdb_str[1].len);
    m_conninfo += " ";

    m_conninfo += "user=";
    m_conninfo += TIMESCALEDB_USER;
    m_conninfo += " ";

    m_conninfo += "password=";
    m_conninfo += TIMESCALEDB_PASSWD;
    //m_conninfo = " ";

    disconnect();
    m_conn = PQconnectdb(m_conninfo.c_str());
    if (PQstatus(m_conn) == CONNECTION_BAD) {
        TSDB_ERROR( p, "[TIMESCALEDB][server=%s]Connection to database failed:%s", server, PQerrorMessage(m_conn) );
        disconnect();
        return EINVAL;
    }

    int ver = PQserverVersion(m_conn);

    TSDB_INFO(p, "[TIMESCALEDB]Server version: %d\n", ver);

    return 0;
}

void wide_timescaledb_conn_t::disconnect()
{
    if ( m_conn ) {
        PQfinish( m_conn );
        m_conn = NULL;
    }
}

int wide_timescaledb_conn_t::select_db( const char * db )
{
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    disconnect();
    std::string conninfo = m_conninfo;
    conninfo += " ";
    conninfo += "dbname=";

    // 如果谁在维护代码 下面这三行不要去掉  
    // 我是故意这样写的  
    // 原因是：postgressql(timescaledb数据库名默认都是小写的 此处应该有个bug 反正大写说是错误 改成小写就可以通过了)  
    std::string s_db = db;
    std::transform(s_db.begin(),s_db.end(),s_db.begin(),::tolower);
    conninfo += s_db;
    

    m_conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(m_conn) == CONNECTION_BAD) {
        TSDB_ERROR(p, "[TIMESCALEDB][server=%s]Connection to database failed:%s", m_conninfo.c_str(), PQerrorMessage(m_conn));
        disconnect();
        return EINVAL;
    }

    
    {
        if (!s_m_is_timescaledb_enable) {
            // 是否已经初始化了timescaledb插件  
            scope_lock_t scope(s_m_lockable);
            if (!s_m_is_timescaledb_enable) {
                // 启用 timescaledb插件  
                // 目前这个插件整个进程内只需要执行一次就可以了  
                // PGresult* res = PQexec(m_conn, "create extension timescaledb;");
                PGresult* res = PQexec(m_conn, "CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;");
                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    TSDB_ERROR(p, "[TIMESCALEDB][server=%s] enable timescaledb plugin failed:(%s) %s",
                        m_conninfo.c_str(), PQresStatus(PQresultStatus(res)), PQerrorMessage(m_conn));
                    PQclear(res);
                    disconnect();
                    return EINVAL;
                }

                PQclear(res);
                res = NULL;
                s_m_is_timescaledb_enable = true;
            }
        }
    }
    
    int ver = PQserverVersion(m_conn);

    TSDB_INFO(p, "[TIMESCALEDB]Server version: %d\n", ver);

    // 冗余检验 查看当前是否操作是的当前数据库  
    char *db_name = PQdb(m_conn);
    if (0 != strcmp(db_name, s_db.c_str()))
    {
        TSDB_INFO(p, "[TIMESCALEDB][input_db:%s][quer_db:%s] not equal", s_db.c_str(), db_name);
        return EINVAL;
    }


    return 0;
}

int wide_timescaledb_conn_t::query_non_result( const char * sql, size_t sql_len )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    const char* my_sql = NULL;
    // 优化 防止字符串越界 如果是以\0结尾 本身就是字符串 不用再转化了  
    if ('\0' != *(sql+sql_len)) {
        m_qureysql.resize(sql_len + 1);
        m_qureysql.assign(sql, sql_len);
        my_sql = m_qureysql.c_str();
    }
    else {
        my_sql = sql;
    }

    PGresult* res = PQexec(m_conn, my_sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
#if 0         
        char* db = PQdb(m_conn);

        ExecStatusType type = PQresultStatus(res);
        char* err = PQerrorMessage(m_conn);
        if (type == PGRES_FATAL_ERROR)
        {
            const char* err_head = "数据库";
            const char* err_tail = "已经存在";
            int comp1 = strncmp(err, err_head, strlen(err_head));
            int comp2 = strncmp(err + strlen(err) - strlen(err_tail), err_tail, strlen(err_tail));
        }
#endif
        TSDB_ERROR(p, "[TIMESCALEDB] query_non_result failed:(%s) %s", PQresStatus(PQresultStatus(res)), PQerrorMessage(m_conn));
        p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, my_sql, sql_len);
        PQclear(res);
        disconnect();
        return EINVAL;
    }

    PQclear(res);
    res = NULL;

    return 0;
}

int wide_timescaledb_conn_t::query_has_result( const char * sql, size_t sql_len, uint64_t & row_count )
{
    row_count = 0;

    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    const char* my_sql = NULL;
    // 优化 防止字符串越界 如果是以\0结尾 本身就是字符串 不用再转化了  
    if ('\0' != *(sql + sql_len)) {
        m_qureysql.resize(sql_len + 1);
        m_qureysql.assign(sql, sql_len);
        my_sql = m_qureysql.c_str();
    }
    else {
        my_sql = sql;
    }


    PGresult * res = PQexec(m_conn, my_sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        TSDB_ERROR(p, "[TIMESCALEDB] query_has_result failed:(%s) %s", PQresStatus(PQresultStatus(res)), PQerrorMessage(m_conn));
        p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, my_sql, sql_len);
        PQclear(res);
        disconnect();
        return EINVAL;
    }

    row_count = PQntuples(res);

    PQclear(res);
    res = NULL;

    return 0;
}

} // namespace wide
} // namespace test
} // namespace rtdb

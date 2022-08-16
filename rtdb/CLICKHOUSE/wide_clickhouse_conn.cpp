#include <sstream>
#include "wide_clickhouse_conn.h"

#if ENABLE_CLICKHOUSE

#define CLICKHOUSE_USER   "default"
#define CLICKHOUSE_PASSWD "default"

namespace rtdb
{

namespace wide
{

wide_clickhouse_conn_t::wide_clickhouse_conn_t()
    : m_conn(NULL),m_qureysql()
{
    m_qureysql.reserve(256 * 1024); // 256K 
    m_qureysql.resize(0);
}

wide_clickhouse_conn_t::~wide_clickhouse_conn_t()
{
    disconnect();
    m_qureysql.resize(0);
}

void wide_clickhouse_conn_t::kill_me()
{
    delete this;
}

int wide_clickhouse_conn_t::connect( const char * server )
{
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    int count = 0;
    p->tools->to_const_array(server, (int)strlen(server), ":", 1, NULL, &count);
    if (count < 2) {
        TSDB_ERROR(p, "[CLICKHOUSE][server=%s]invalid server string", server);
        return EINVAL;
    }

    std::vector<tsdb_str> vt_tsdb_str;
    vt_tsdb_str.resize(count);
    p->tools->to_const_array(server, (int)strlen(server), ":", 1, &vt_tsdb_str[0], &count);
    if (count < 2) {
        TSDB_ERROR(p, "[CLICKHOUSE][server=%s]invalid server string", server);
        return EINVAL;
    }

    clickhouse::ClientOptions co;
    
    const std::string ip = std::string(vt_tsdb_str[0].ptr, vt_tsdb_str[0].len);
    const std::string port_string = std::string(vt_tsdb_str[1].ptr, vt_tsdb_str[1].len);
    const std::string user = (count >= 3 ? std::string(vt_tsdb_str[2].ptr, vt_tsdb_str[2].len) : CLICKHOUSE_USER);
    const std::string password = (count >= 4 ? std::string(vt_tsdb_str[3].ptr, vt_tsdb_str[3].len) : CLICKHOUSE_PASSWD);;
    const unsigned int port = atoi(port_string.c_str());

    try
    {
        co.SetHost(ip).SetPort(port).SetUser(user).SetPassword(password);
        m_conn = new clickhouse::Client(co);
    }
    catch (...)
    {
        TSDB_ERROR(p, "[CLICKHOUSE][server=%s][port:%d][user:%s][passeord:%s] connect clickhouse failed or no mem", 
            ip.c_str(), (int)port, CLICKHOUSE_USER, CLICKHOUSE_PASSWD);
        return ENOMEM;
    }
    

    return 0;
}

void wide_clickhouse_conn_t::disconnect()
{
    if ( m_conn ) {
        delete m_conn;
        m_conn = NULL;
    }
}

int wide_clickhouse_conn_t::select_db( const char * db )
{
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    
    std::string s_db = "use ";
    s_db += db;
    s_db += ";";

    try
    {
        m_conn->Execute(s_db);
    }
    catch (...)
    {
        TSDB_ERROR(p, "[CLICKHOUSE][sql:%s] use db failed", s_db.c_str());
        return EFAULT;
    }

    return 0;
}

int wide_clickhouse_conn_t::query_non_result( const char * sql, size_t sql_len )
{
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

    try
    {
        m_conn->Execute(my_sql);
    }
    catch (...)
    {
        TSDB_ERROR(p, "[CLICKHOUSE] query_non_result failed");
        //p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, my_sql, sql_len);
        disconnect();
        return EFAULT;
    }

    return 0;
}

int wide_clickhouse_conn_t::query_has_result( const char * sql, size_t sql_len, uint64_t & row_count )
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


    try
    {
        /// Select values inserted in the previous step.
        m_conn->Select(my_sql, [&row_count](const clickhouse::Block& block) {
            row_count += block.GetRowCount();
            }
        );
    }
    catch (...)
    {
        TSDB_ERROR(p, "[CLICKHOUSE] query_has_result failed");
        p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, my_sql, sql_len);
        disconnect();
        return EFAULT;
    }
   

    return 0;
}

} // namespace wide

} // namespace rtdb
#endif // #if ENABLE_CLICKHOUSE

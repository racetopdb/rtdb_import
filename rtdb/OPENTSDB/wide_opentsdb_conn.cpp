#include "wide_opentsdb_conn.h"


#if ENABLE_OPENTSDB
#include "../HTTP.h"  // opentsdb Database Interface.
#include "cJSON.h"
#endif // #if ENABLE_OPENTSDB




namespace rtdb
{

namespace wide
{

wide_opentsdb_conn_t::wide_opentsdb_conn_t()
    : m_conn( NULL )
{
}

wide_opentsdb_conn_t::~wide_opentsdb_conn_t()
{
    disconnect();
}

void wide_opentsdb_conn_t::kill_me()
{
    delete this;
}

int wide_opentsdb_conn_t::connect( const char * server )
{
    int r = 0;
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    m_conn = new HTTP();

    r = m_conn->initCurl(server, true,  server, true);
    if (0 != r) {
        TSDB_ERROR(p, "[opentsdb][server=%s]invalid server string", server);
        return r;
    }


    return 0;
}

void wide_opentsdb_conn_t::disconnect()
{
    if ( m_conn ) {
        m_conn->close();
        m_conn = NULL;
    }
}

int wide_opentsdb_conn_t::select_db( const char * db )
{
    return 0;
}

int wide_opentsdb_conn_t::query_non_result( const char * sql, size_t sql_len )
{
    int r = 0;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    std::string* returnHeader = NULL;
    std::string* returnBody = NULL;
    const std::string appendUrl = "/api/put";
    const std::string& postBody = std::string(sql, sql_len);
    r = m_conn->post(appendUrl, postBody, returnHeader, returnBody);
    if ( r != 204 ) {
        TSDB_ERROR( p, "[opentsdb][r=%d][header:%s][body:%s] HTTP::post failed", 
            r, 
            (NULL != returnHeader ? returnHeader->c_str() : "null"), 
            (NULL != returnBody ?  returnBody->c_str() : "null"));
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return EINVAL;
    }
    
    return 0;
}

int wide_opentsdb_conn_t::query_has_result( const char * sql, size_t sql_len, uint64_t & row_count )
{
    int r = 0;
    row_count = 0;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );


    std::string* returnHeader = NULL;
    std::string* returnBody = NULL;
    const std::string appendUrl = "/api/query";
    const std::string& postBody = std::string(sql, sql_len);
    r = m_conn->post(appendUrl, postBody, returnHeader, returnBody);
    if (r != 200) {
        TSDB_ERROR( p, "[opentsdb][r=%d][header:%s][body:%s] HTTP::post failed", r, returnHeader->c_str(), returnBody->c_str());
        p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len);
        return EINVAL;
    }

    row_count = get_row_count(returnBody);

    return 0;
}

int64_t wide_opentsdb_conn_t::get_row_count(std::string* returnBody) 
{
    int64_t row_count = 0;
    rtdb::cJSON *json = NULL; 

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    //解析成json形式  
    json = rtdb::cJSON_Parse(returnBody->c_str()); 
    if (NULL == json) {
        TSDB_ERROR(p, "[opentsdb][json:%s] cJSON_Parse is NULL not support failed", returnBody->c_str());
        return 0;
    }
    
    int size = rtdb::cJSON_GetArraySize(json);
    for (int i = 0; i < size; i++)
    {
        rtdb::cJSON *arrya_item = rtdb::cJSON_GetArrayItem(json, i);
        if (rtdb::cJSON_HasObjectItem(arrya_item, "dps")) {
            rtdb::cJSON* dps = rtdb::cJSON_GetObjectItem(arrya_item, "dps");
            if (!rtdb::cJSON_IsNull(dps)) {
                rtdb::cJSON* child = NULL;
                child = dps->child;
                while (child) {
                    if (rtdb::cJSON_IsNumber(child)) {
                        row_count += (int64_t)rtdb::cJSON_GetNumberValue(child);
                    }
                    child = child->next;
                }
            }
        }
    }

    if (NULL != json) {
        rtdb::cJSON_Delete(json);
        json = NULL;
    }

    return row_count;
}


} // namespace wide

} // namespace rtdb

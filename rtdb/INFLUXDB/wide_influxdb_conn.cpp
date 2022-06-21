#include "wide_influxdb_conn.h"


#if ENABLE_INFLUXDB
#include "../HTTP.h"  // influxdb Database Interface.
#include "cJSON.h"
#endif // #if ENABLE_INFLUXDB




namespace rtdb
{

namespace wide
{

wide_influxdb_conn_t::wide_influxdb_conn_t()
    : m_conn( NULL ), m_db()
{
}

wide_influxdb_conn_t::~wide_influxdb_conn_t()
{
    disconnect();
}

void wide_influxdb_conn_t::kill_me()
{
    delete this;
}

int wide_influxdb_conn_t::connect( const char * server )
{
    int r = 0;
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    m_conn = new HTTP();

    r = m_conn->initCurl(server, false,  server, false);
    if (0 != r) {
        TSDB_ERROR(p, "[influxdb][server=%s]invalid server string", server);
        return r;
    }


    return 0;
}

void wide_influxdb_conn_t::disconnect()
{
    if ( m_conn ) {
        m_conn->close();
        m_conn = NULL;
    }
    m_db.resize(0);
}

int wide_influxdb_conn_t::select_db( const char * db )
{
    int r = 0;
    m_db = db;
    

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    std::string* returnHeader = NULL;
    std::string* returnBody = NULL;
    const std::string appendUrl = "query?&q=";
    //std::string postBody = "q=";
    std::string postBody = "";
    postBody += "CREATE DATABASE";
    postBody += " ";
    postBody += db;

    r = m_conn->get(appendUrl, postBody, true, returnHeader, returnBody);
    if (r != 200) {
        TSDB_ERROR(p, "[influxdb][r=%d][header:%s][body:%s] HTTP::post failed", 
            r,
            (NULL != returnHeader ?  returnHeader->c_str() : "null"),
            (NULL != returnBody ? returnBody->c_str() : "null")
        );
        return EINVAL;
    }

    return 0;
}

int wide_influxdb_conn_t::query_non_result( const char * sql, size_t sql_len )
{
    int r = 0;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    std::string* returnHeader = NULL;
    std::string* returnBody = NULL;

    std::string appendUrl = "/write?db=";
    appendUrl += m_db;

    const std::string& postBody = std::string(sql, sql_len);
    r = m_conn->post(appendUrl, postBody, returnHeader, returnBody);
    if ( r != 204 ) {
        TSDB_ERROR( p, "[influxdb][r=%d][header:%s][body:%s] HTTP::post failed", 
            r, 
            (NULL != returnHeader ? returnHeader->c_str() : "null"), 
            (NULL != returnBody ?  returnBody->c_str() : "null"));
        p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len );
        return EINVAL;
    }
    
    return 0;
}

int wide_influxdb_conn_t::query_has_result( const char * sql, size_t sql_len, uint64_t & row_count )
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
    std::string appendUrl = "query?";
    appendUrl += "db=";
    appendUrl += m_db;
    appendUrl += "&";
    appendUrl += "pretty=true";
    appendUrl += "&q=";
    

    const std::string& postBody = std::string(sql, sql_len);
    r = m_conn->get(appendUrl, postBody, true, returnHeader, returnBody);
    if (r != 200) {
        TSDB_ERROR( p, "[influxdb][r=%d][header:%s][body:%s] HTTP::post failed", r, returnHeader->c_str(), returnBody->c_str());
        p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql, sql_len);
        return EINVAL;
    }

    row_count = get_row_count(returnBody);

    return 0;
}

int64_t wide_influxdb_conn_t::get_row_count(std::string* returnBody) 
{

    int64_t row_count = 0;
    rtdb::cJSON *json = NULL; 

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    //解析成json形式  
    json = rtdb::cJSON_Parse(returnBody->c_str()); 
    if (NULL == json) {
        TSDB_ERROR(p, "[influxdb][json:%s] cJSON_Parse is NULL not support failed", returnBody->c_str());
        return 0;
    }
    
    rtdb::cJSON *results = rtdb::cJSON_GetObjectItem(json, "results");
    if (NULL == results) {
        if (NULL != json) {
            rtdb::cJSON_Delete(json);
            json = NULL;
        }
        return 0;
    }

    int size = rtdb::cJSON_GetArraySize(results);
    for (int i = 0; i < size; i++)
    {
        rtdb::cJSON *result_item = rtdb::cJSON_GetArrayItem(results, i);
        if (rtdb::cJSON_HasObjectItem(result_item, "series")) {
            rtdb::cJSON* series = rtdb::cJSON_GetObjectItem(result_item, "series");
            if (!rtdb::cJSON_IsNull(series)) {
                int series_size = rtdb::cJSON_GetArraySize(series);
                for (int j = 0; j < series_size; j++) {
                    rtdb::cJSON *serie_item = rtdb::cJSON_GetArrayItem(series, i);
                    if (NULL != serie_item) {
                        rtdb::cJSON* values = rtdb::cJSON_GetObjectItem(serie_item, "values");
                        if (NULL != values) {
                            row_count += rtdb::cJSON_GetArraySize(values);
                        }
                    }
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

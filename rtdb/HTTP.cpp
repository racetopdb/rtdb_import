/**
 * Copyright (C)2020 RTDB. All Rights Reserved  
 * @file HTTP.h  
 * @author yuhaijun(yuhaijun@rtdb.com)  
 * @date 2022-05-26 15:59:29  
 * @version 1.0.0.0  
 * @brief   curl 简单封装 主要是给后续 influxdb 和 opentsdb 使用   
 * Revision History  
 *
 * @if  ID       Author       Date          Major Change       @endif  
 *  ---------+------------+------------+------------------------------+
 *       1     yuhaijun   2022-05-26 15:59:29   Created.  
 **/
#include "HTTP.h"
#include "wide_base.h"

namespace rtdb
{
namespace wide
{


static size_t callback(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string*>((std::string*)lpVoid);
    if (NULL == str || NULL == buffer)
    {
        return -1;
    }
    str->append((char*)buffer, size * nmemb);
    return size * nmemb;
}

/**
* @brief  全局初始化 仅仅初始化一次即可  建议主线程调用  
* @return  成功 ： 0 其他 错误    
*/
int HTTP::init_once()
{
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    CURLcode globalInitResult = curl_global_init(CURL_GLOBAL_ALL);
    if (globalInitResult != CURLE_OK) {
        TSDB_ERROR(p, "[HTTP][r=%d][error:%s] curl_global_init failed", (int)globalInitResult, curl_easy_strerror(globalInitResult));
        return globalInitResult;
    }
    return (int)CURLE_OK;
}


/**
* @brief  全局销毁 仅仅销毁一次即可   建议主线程调用  
* @return  无  
*/
void HTTP::destroy_once()
{
    curl_global_cleanup();
}

/**
* @brief  curl HTTP 构造  
* @return  无  
*/
HTTP::HTTP() :
    m_postHandle(NULL)
    , m_getHandle(NULL)
    , m_postUrl()
    , m_getUrl()
    , m_postBuffer()
    , m_postHeaderBuffer()
    , m_getBuffer()
    , m_getHeaderBuffer()
{

}

/**
* @brief  curl HTTP 初始化  
* @param[in]  const std::string& postUrl       post url 此url为请求的公共url  
* @param[in]  isPostJson                       是否是json格式  
* @param[in]  const std::string& getUrl        get url 此url为请求的公共url  
* @param[in]  bool isGetJson                   是否是json格式  
* @return  成功 ： 0 其他 错误    
*/
int HTTP::initCurl(const std::string& postUrl, bool isPostJson, const std::string& getUrl, bool isGetJson)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    r = initCurlPost(postUrl, isPostJson);
    if (0 != r) {
        TSDB_ERROR(p, "[HTTP][r=%d]initCurlPost failed", r);
        return r;
    }

    r = initCurlGet(getUrl, isGetJson);
    if (0 != r) {
        TSDB_ERROR(p, "[HTTP][r=%d]initCurlGet failed", r);
        return r;
    }

    

    // m_postHandle(NULL)
    // m_getHandle(NULL)
    // m_postUrl()
    // m_getUrl()
    m_postBuffer.resize(2048);
    m_postHeaderBuffer.resize(1024);
    m_getBuffer.resize(10*1024);
    m_getHeaderBuffer.resize(1024);

    return 0;
}

/**
* @brief  curl HTTP 关闭  
* @return  无  
*/
void HTTP::close()
{
    if (NULL != m_postHandle) {
        curl_easy_cleanup(m_postHandle);
        m_postHandle = NULL;
    }
    
    if (NULL != m_getHandle) {
        curl_easy_cleanup(m_getHandle);
        m_getHandle = NULL;
    }

    m_postUrl.resize(0);
    m_getUrl.resize(0);
    m_postBuffer.resize(0);
    m_postHeaderBuffer.resize(0);
    m_getBuffer.resize(0);
    m_getHeaderBuffer.resize(0);

    return;
}


/**
* @brief  curl HTTP 销毁  
* @param[in]  无  
* @return  无  
*/
HTTP::~HTTP()
{
    close();
}


/**
* @brief  curl HTTP POST 请求  
* @param[in]  const std::string &appendUrl   附加url  
* @param[in]  const std::string& postBody    post体  
* @param[out] std::string *&returnHeader     返回请求头  
* @param[out] std::string *&returnBody       返回请求体（可能没有内容）  
* @return  成功：200，204 都可认为是成功   失败：负数 curl内部错误 其他为http 响应码  
*/
int HTTP::post(const std::string &appendUrl, const std::string& postBody, std::string *&returnHeader, std::string *&returnBody)
{
    CURLcode response = CURLE_OK;
    long responseCode = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    std::string fullUrl;
    
    if ((!m_postUrl.empty()) && '/' == (*(m_postUrl.end() - 1))) {
        if ((!appendUrl.empty()) && '/' == (*(appendUrl.begin()))) {
            fullUrl = m_postUrl + appendUrl.substr(1);
        }
        else {
            fullUrl = m_postUrl + appendUrl;
        }
    }
    else {
        fullUrl = m_postUrl + appendUrl;
    }


    m_postBuffer.resize(0);
    m_postHeaderBuffer.resize(0);

    curl_easy_setopt(m_postHandle, CURLOPT_URL, fullUrl.c_str());

    curl_easy_setopt(m_postHandle, CURLOPT_POSTFIELDS, postBody.c_str());
    curl_easy_setopt(m_postHandle, CURLOPT_POSTFIELDSIZE, (long)postBody.length());
    response = curl_easy_perform(m_postHandle);
    curl_easy_getinfo(m_postHandle, CURLINFO_RESPONSE_CODE, &responseCode);
    if (response != CURLE_OK) {
        TSDB_ERROR(p, "[HTTP][r=%d][error:%s] POST failed", (int)response, curl_easy_strerror(response));
        return (int)-response;
    }
    returnHeader =  &m_postHeaderBuffer;
    returnBody = &m_postBuffer;

    if (responseCode < 200 || responseCode > 206) {
        TSDB_ERROR(p, "[HTTP][r=%d][responseCode:%d] [error:%s]  POST failed", (int)response, (int)responseCode,  curl_easy_strerror(response));
        return responseCode;
    }
    return responseCode;
}



/**
* @brief  curl HTTP GET 请求  
* @param[in]  const std::string &appendUrl   附加url  
* @param[in]  const std::string& getBody    post体  
* @param[in]  bool isConverCode              是否转码  
* @param[out] std::string *&returnHeader     返回请求头  
* @param[out] std::string *&returnBody       返回请求体  
* @return  成功：200，204 都可认为是成功   失败：负数 curl内部错误 其他为http 响应码  
*/
int HTTP::get(const std::string& appendUrl, const std::string& getBody, bool isConverCode, std::string*& returnHeader, std::string*& returnBody)
{
    CURLcode response = CURLE_OK;
    long responseCode = 0;
    std::string buffer;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    std::string fullUrl;
    char* encodedQuery = NULL;

    if (isConverCode) {
        encodedQuery = curl_easy_escape(m_getHandle, getBody.c_str(), (int)getBody.size());
        if ((!m_getUrl.empty()) && '/' == (*(m_getUrl.end() - 1))) {
            fullUrl = m_getUrl + appendUrl + encodedQuery;
        }
        else {
            fullUrl = m_getUrl + "/" + appendUrl + encodedQuery;
        }
        curl_free(encodedQuery);
    }
    else {
        if ((!m_getUrl.empty()) && '/' == (*(m_getUrl.end() - 1))) {
            fullUrl = m_getUrl + appendUrl + getBody;
        }
        else {
            fullUrl = m_getUrl + "/" + appendUrl + getBody;
        }
    }

    m_getBuffer.resize(0);
    m_getHeaderBuffer.resize(0);

    curl_easy_setopt(m_getHandle, CURLOPT_URL, fullUrl.c_str());
    response = curl_easy_perform(m_getHandle);
    curl_easy_getinfo(m_getHandle, CURLINFO_RESPONSE_CODE, &responseCode);

    returnHeader = &m_getHeaderBuffer;
    returnBody = &m_getBuffer;

    if (response != CURLE_OK) {
        TSDB_ERROR(p, "[HTTP][r=%d][error:%s] GET failed", (int)response, curl_easy_strerror(response));
        return (int)-response;
    }
    if (responseCode != 200) {
        TSDB_ERROR(p, "[HTTP][r=%d][responseCode:%d] [error:%s]  POST failed", (int)response, (int)responseCode, curl_easy_strerror(response));
        return responseCode;
    }

    return responseCode;
}


/**
* @brief  curl HTTP 启用权限验证  目前不支持  
* @param[in] const std::string& auth              权限  
* @return  无  
*/
void HTTP::enableBasicAuth(const std::string& auth)
{
    //curl_easy_setopt(m_postHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    //curl_easy_setopt(m_postHandle, CURLOPT_USERPWD, auth.c_str());
    //curl_easy_setopt(m_getHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    //curl_easy_setopt(m_getHandle, CURLOPT_USERPWD, auth.c_str());
    return;
}

/**
* @brief  curl HTTP 启用ssl  目前不支持  
* @param[in] 无  
* @return  无  
*/
void HTTP::enableSsl()
{
    //curl_easy_setopt(m_postHandle, CURLOPT_SSL_VERIFYPEER, 0L);
    //curl_easy_setopt(m_getHandle, CURLOPT_SSL_VERIFYPEER, 0L);
    return;
}



/**
* @brief  初始化 curl post 请求  
* @param[in]  const std::string& url   url  
* @param[in]  bool isJson              是否是json格式  
* @return  成功 ： 0 其他 错误    
*/
int HTTP::initCurlPost(const std::string& url, bool isJson)
{
    m_postUrl = url;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);
  
    try
    {
        m_postHandle = curl_easy_init();
    }
    catch (const std::exception&)
    {
        TSDB_ERROR(p, "[HTTP] alloc for CURL faield");
        return ENOMEM;
    }

    if (NULL == m_postHandle) {
        TSDB_ERROR(p, "[HTTP] alloc for CURL faield");
        return ENOMEM;
    }

    curl_easy_setopt(m_postHandle, CURLOPT_URL, m_postUrl.c_str());
    curl_easy_setopt(m_postHandle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(m_postHandle, CURLOPT_CONNECTTIMEOUT, 10000000);
    curl_easy_setopt(m_postHandle, CURLOPT_TIMEOUT, 10000000);
    curl_easy_setopt(m_postHandle, CURLOPT_POST, 1);
    curl_easy_setopt(m_postHandle, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(m_postHandle, CURLOPT_TCP_KEEPINTVL, 60L);
    if (isJson) {
        struct curl_slist* hs = NULL;
        hs = curl_slist_append(hs, "Content-Type: application/json");
        curl_easy_setopt(m_postHandle, CURLOPT_HTTPHEADER, hs);
        // TODO : 不知道该不该释放hs  
    }

    curl_easy_setopt(m_postHandle, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(m_postHandle, CURLOPT_WRITEDATA, &m_postBuffer);

    curl_easy_setopt(m_postHandle, CURLOPT_HEADERFUNCTION, callback);
    curl_easy_setopt(m_postHandle, CURLOPT_HEADERDATA, &m_postHeaderBuffer);

    return 0;
}

/**
* @brief  初始化 curl get 请求  
* @param[in]  const std::string& url   url  
* @param[in]  bool isJson              是否是json格式  
* @return  成功 ： 0 其他 错误    
*/
int HTTP::initCurlGet(const std::string& url, bool isJson) 
{
    m_getUrl = url;
 
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    try
    {
        m_getHandle = curl_easy_init();
    }
    catch (const std::exception&)
    {
        TSDB_ERROR(p, "[HTTP] alloc for CURL faield");
        return ENOMEM;
    }

    if (NULL == m_getHandle) {
        TSDB_ERROR(p, "[HTTP] alloc for CURL faield");
        return ENOMEM;
    }

    curl_easy_setopt(m_getHandle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(m_getHandle, CURLOPT_CONNECTTIMEOUT, 10000000);
    curl_easy_setopt(m_getHandle, CURLOPT_TIMEOUT, 10000000);
    curl_easy_setopt(m_getHandle, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(m_getHandle, CURLOPT_TCP_KEEPINTVL, 60L);

    if (isJson) {
        struct curl_slist* hs = NULL;
        hs = curl_slist_append(hs, "Content-Type: application/json");
        curl_easy_setopt(m_getHandle, CURLOPT_HTTPHEADER, hs);
        // TODO : 不知道该不该释放hs  
    }

    curl_easy_setopt(m_getHandle, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(m_getHandle, CURLOPT_WRITEDATA, &m_getBuffer);

    curl_easy_setopt(m_getHandle, CURLOPT_HEADERFUNCTION, callback);
    curl_easy_setopt(m_getHandle, CURLOPT_HEADERDATA, &m_getHeaderBuffer);
    return 0;
}



#ifdef _MSC_VER
#if defined( _WIN64 )
#pragma message( "build CURL test wide table on X86_64" )
#pragma comment( lib, "CURL/x64/libcurl.lib" )
#elif defined( _WIN32 )
#pragma message( "build CURL test wide table on Win32" )
#pragma comment( lib, "CURL/win32/libcurl.lib" )
#endif
#endif

} // namespace wide
} // namespace rtdb

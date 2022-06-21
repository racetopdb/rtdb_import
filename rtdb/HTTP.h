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
#ifndef _rtdb_test_wide_base_HTTP_h_
#define _rtdb_test_wide_base_HTTP_h_
#include <curl/curl.h>
#include <memory>
#include <string>

namespace rtdb
{
namespace wide
{

// CURL get post 简单封装类  
class HTTP
{
public:
    
    /**
    * @brief  全局初始化 仅仅初始化一次即可  建议主线程调用     
    * @return  成功 ： 0 其他 错误    
    */
    static int init_once();

    /**
    * @brief  全局销毁 仅仅销毁一次即可   建议主线程调用  
    * @return  无  
    */
    static void destroy_once();

    /**
    * @brief  curl HTTP 构造  
    * @return  无  
    */
    HTTP();

    /**
    * @brief  curl HTTP 初始化  
    * @param[in]  const std::string& postUrl       post url 此url为请求的公共url  
    * @param[in]  isPostJson                       是否是json格式  
    * @param[in]  const std::string& getUrl        get url 此url为请求的公共url  
    * @param[in]  bool isGetJson                   是否是json格式  
    * @return  成功 ： 0 其他 错误    
    */
    int initCurl(const std::string& postUrl, bool isPostJson,  const std::string& getUrl, bool isGetJson);

    
    /**
    * @brief  curl HTTP 关闭  
    * @return  无  
    */
    void close();

    /**
    * @brief  curl HTTP 销毁  
    * @param[in]  无  
    * @return  无  
    */
    ~HTTP();

  
    /**
    * @brief  curl HTTP POST 请求  
    * @param[in]  const std::string &appendUrl   附加url  
    * @param[in]  const std::string& postBody    post体  
    * @param[out] std::string *&returnHeader     返回请求头  
    * @param[out] std::string *&returnBody       返回请求体（可能没有内容）  
    * @return  成功：200，204 都可认为是成功   失败：负数 curl内部错误 其他为http 响应码  
    */
    int post(const std::string &appendUrl, const std::string& postBody, std::string *&returnHeader, std::string *&returnBody);

   
    
    /**
    * @brief  curl HTTP GET 请求  
    * @param[in]  const std::string &appendUrl   附加url  
    * @param[in]  const std::string& getBody    post体  
    * @param[in]  bool isConverCode              是否转码  
    * @param[out] std::string *&returnHeader     返回请求头  
    * @param[out] std::string *&returnBody       返回请求体  
    * @return  成功：200，204 都可认为是成功   失败：负数 curl内部错误 其他为http 响应码  
    */
    int get(const std::string &appendUrl, const std::string& getBody,bool isConverCode, std::string *&returnHeader, std::string *&returnBody);


    /**
    * @brief  curl HTTP 启用权限验证  目前不支持  
    * @param[in] const std::string& auth              权限  
    * @return  无  
    */
    void enableBasicAuth(const std::string& auth);

    /**
    * @brief  curl HTTP 启用ssl  目前不支持  
    * @param[in] 无  
    * @return  无  
    */
    void enableSsl();
private:

    
    /**
    * @brief  初始化 curl post 请求  
    * @param[in]  const std::string& url   url  
    * @param[in]  bool isJson              是否是json格式  
    * @return  成功 ： 0 其他 错误  
    */
    int initCurlPost(const std::string& url, bool isJson);

    /**
    * @brief  初始化 curl get 请求  
    * @param[in]  const std::string& url   url  
    * @param[in]  bool isJson              是否是json格式  
    * @return  成功 ： 0 其他 错误  
    */
    int initCurlGet(const std::string& url, bool isJson);

    
    /// curl post 句柄  
    CURL* m_postHandle;

    /// curl get 句柄  
    CURL* m_getHandle;

    /// post url 固定部分  
    std::string m_postUrl;

    /// get url 固定部分  
    std::string m_getUrl;

    /// post body buffer 外部使用仅只读  
    std::string m_postBuffer;

    /// post header buffer 外部使用仅只读  
    std::string m_postHeaderBuffer;

    /// get buffer 外部使用仅只读  
    std::string m_getBuffer;

    /// get header buffer 外部使用仅只读  
    std::string m_getHeaderBuffer;
};

} // namespace wide
} // namespace rtdb

#endif // _rtdb_test_wide_base_HTTP_h_

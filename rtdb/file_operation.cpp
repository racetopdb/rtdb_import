#include "file_operation.h"


using namespace rtdb::wide;


rtdb::wide::file_operation::
file_operation() :
    m_stream(NULL), m_filename()
{
}

rtdb::wide::file_operation::
~file_operation()
{
    close();
}

/**
 * @brief 读方式打开  
 * @param[in]  const char *filename    文件名  
 * @return    0 成功 其他 失败  
 */
int rtdb::wide::file_operation::open_by_read(const char* filename)
{
    FILE* stream = fopen(filename, "rb");
    if (unlikely(NULL == stream)) {
        return ENOENT;
    }

    m_filename = filename;
    m_stream = stream;

    return 0;
}

/**
 * @brief 写方式打开  
 * @param[in]  const char *filename    文件名  
 * @return    0 成功 其他 失败  
 */
int rtdb::wide::file_operation::open_by_write(const char* filename)
{
    FILE* stream = fopen(filename, "wb");
    if (unlikely(NULL == stream)) {
        return ENOENT;
    }

    m_filename = filename;
    m_stream = stream;

    return 0;
}

/**
 * @brief   获取一行数据  
 * @param[in][out]  char buffer[]                buffer 缓存  
 * @param[in]       unsigned int buffer_bytes    buffer 缓存大小  
 * @return  >=0 成功 -1 失败  
 */
int rtdb::wide::file_operation::read_one_line(char buffer[], unsigned int buffer_bytes)
{
    char* pbuffer = NULL;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    pbuffer = fgets(buffer, buffer_bytes, m_stream);
    if (unlikely(NULL == pbuffer)) {
        int end_of_file = feof(m_stream);
        //<private> 如果出错了 //</private>  
        if (likely(0 == end_of_file)) {
            TSDB_ERROR(p, "[forward_iterator][file:%s] error", m_filename.data());
            return -EIO;
        }

        return 0;
    }

    return (int)(strlen(buffer));
}


/**
 * @brief   写一行数据  
 * @param[in]  const char buffer[]          buffer 缓存  
 * @param[in]  unsigned int buffer_bytes    buffer 缓存大小  
 * @param[in]  bool is_flush                     是否强制刷新缓存  
 * @return  >=0 成功 -1 失败  
 */
int rtdb::wide::file_operation::write_one_line(
    const char buffer[], unsigned int buffer_bytes, bool is_flush)
{
    size_t  size = fwrite((const void*)buffer, 1, (size_t)buffer_bytes, m_stream);
    if (unlikely(0 == size)) {
        return EINVAL;
    }

    if (is_flush) {
        fflush(m_stream);
    }


    return (int)size;
}

/**
 * @brief  关闭  
 * @return  0 成功 其他 失败  
 */
int rtdb::wide::file_operation::close()
{
    if (likely(NULL != m_stream)) {
        fclose(m_stream);
        m_stream = NULL;
    }

    m_filename.clear();
    return 0;
}

#ifndef _rtdb_test_wide_file_operation_h_
#define _rtdb_test_wide_file_operation_h_

#include "wide_base.h"

namespace rtdb
{
namespace wide
{

    class file_operation
    {
    public:
        file_operation();
        virtual ~file_operation();

        /**
         * @brief 读方式打开  
         * @param[in]  const char *filename    文件名  
         * @return    0 成功 其他 失败  
         */
        int open_by_read(const char* filename);


        /**
         * @brief 写方式打开  
         * @param[in]  const char *filename    文件名  
         * @return    0 成功 其他 失败  
         */
        int open_by_write(const char* filename);

        /**
         * @brief   获取一行数据  
         * @param[in][out]  char buffer[]                buffer 缓存  
         * @param[in]       unsigned int buffer_bytes    buffer 缓存大小  
         * @return  >=0 成功 -1 失败  
         */
        int read_one_line(char buffer[], unsigned int buffer_bytes);


        /**
         * @brief   写一行数据  
         * @param[in]  const char buffer[]          buffer 缓存  
         * @param[in]  unsigned int buffer_bytes    buffer 缓存大小  
         * @param[in]  bool is_flush                是否强制刷新缓存 默认不强制刷新  
         * @return  >=0 成功 -1 失败  
         */ 
        int write_one_line(const char buffer[], unsigned int buffer_bytes, bool is_flush = false);


        /**
         * @brief  关闭  
         * @return  0 成功 其他 失败  
         */
        int close();

    protected:
        FILE* m_stream;
        std::string m_filename;

        //file_operation(const file_operation&);
        //const file_operation& operator = (const file_operation&);
    };


} // namespace wide
} // namespace rtdb

#endif

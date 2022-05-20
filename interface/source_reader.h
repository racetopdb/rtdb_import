#ifndef _source_reader_h_
#define _source_reader_h_

#include <vector>
#include <string>

namespace rtdb
{
class field_value_t
{
public:

    field_value_t() : field_name(), field_value() {}
    field_value_t(const field_value_t & rhd) : field_name(rhd.field_name), field_value(rhd.field_value) {}
    const field_value_t & operator = (const field_value_t & rhd)
    {
        if ( this != & rhd ) {
            field_name  = rhd.field_name;
            field_value = rhd.field_value;
        }
        return * this;
    }

    std::string     field_name;

    std::string     field_value;
};

class table_names_t
{
public:

    table_names_t() : names() {}
    table_names_t(const table_names_t & rhd) : names(rhd.names) {}
    const table_names_t & operator = (const table_names_t & rhd)
    {
        if ( this != & rhd ) {
            names       = rhd.names;
        }
        return * this;
    }

    std::vector< std::string >  names;
};

/**
 * @brief 全局数据源  
 */
class import_source_t
{
public:

    import_source_t() {}
    virtual ~import_source_t() {}

    /**
     * @brief 打开一个全局数据源  
     * @param[in] unsigned int worker_count 工作线程数量  
     * @param[in] unsigned void * params    参数指针  由具体的实现定义  
     * @return int 错误码  
     */
    virtual int open( unsigned int worker_count, void * params ) = 0;

    /**
     * @brief 关闭全局数据源  
     */
    virtual void close() = 0;

    /**
     * @brief 取得一个工作者对象，传入工作者线程ID [0, worder_count)  
     * @param[in] unsigned int worker_count 工作线程数量  
     * @return import_worker_t * 调用方不不负其生命周期。如果调用成功，非指针。  
     */
    virtual import_worker_t * get_worker( unsigned int worker_id ) = 0;

    /**
     * @brief 在 open 时传入的 worker_count 
     * @return int 在 open 时传入的 worker_count
     */
    virtual unsigned int get_worker_count() const = 0;

private:
    // disable
    import_source_t(const import_source_t &);
    const import_source_t & operator = (const import_source_t &);
};

/**
 * @brief 工作线程  
 */
class import_worker_t
{
public:

    virtual int read( field_value_t * & values, size_t & values_count, table_names_t * & table_names ) = 0;
    
    virtual int read( std::string & fields, std::string & values, table_names_t * & table_names ) = 0;

protected:

    friend class import_source_t;

    import_worker_t() {}
    virtual ~import_worker_t() {}

    /**
     * @brief 初始化工作线程，本方法由 import_source_t 的 get_worker() 调用。   
     * @param[in] import_source_t * source  全局数据源指针。  
     * @param[in] unsigned int worker_id    当前工作线程ID  
     */
    virtual int open( import_source_t * source, unsigned int worker_id ) = 0;

    /**
     * @brief 关闭工作线程  
     */
    virtual void close() = 0;

    /**
     * @brief 当前工作线程的ID [0, worker_count)
     * @return int 当前工作线程的ID
     */
    virtual unsigned int get_worker_id() const = 0;

private:
    // disable
    import_worker_t(const import_worker_t &);
    const import_worker_t & operator = (const import_worker_t &);
}
} // namespace rtdb

#endif

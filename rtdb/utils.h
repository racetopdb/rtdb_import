#ifndef _rtdb_test_wide_utils_h_
#define _rtdb_test_wide_utils_h_

#include "wide_base.h"
#include "file_operation.h"
#include <map>


namespace rtdb
{

namespace wide
{

#define TSDB_COLUMN_TIME_NAME                   "time"

static const char* s_null_string = "null";

// 默认CSV文件分割符号  
#define DEFAULT_CSV_FILE_SEP "\t"

// 定义 配置数据文件map value 结构  
//      有两部分组成 1.数据文件列表 2.分隔符号  允许为空  
typedef std::vector<std::string> CONFIG_DATA_PATHS_T;
typedef std::pair<CONFIG_DATA_PATHS_T, std::string> CONFIG_DATA_PATHS_SEP_T;

// 定义默认buffer大小  
static const int  DEFAULT_BUFFER_BYTES = 8192;

// 无效但是可以被忽略  
#define EINVALMAYEIGNORE         201


// 字段增加变量结构体  
struct field_increase_store_t {

    // 步长  
    //int step;
    float fstep;

    // 字段增加变量 联合体  
    union fis_data_t
    {
        // int 变量值  
        int value;

        // 浮点数值  
        float fvalue;

        // int64 变量值  
        int64_t value64;
    };

    union fis_data_t data;

    field_increase_store_t() {
        fstep = 0.0f;
        data.value64 = 0;
    }

    ~field_increase_store_t() {
        fstep = 0.0f;
        data.value64 = 0;
    }
};


// 生成表数据运行时信息  
struct generate_table_data_runtime_info_t {

    // 文件路径  
    std::string file_path;

    // 文件是否打开 默认为false  
    bool is_file_open;

    // 文件操作类指针  
    file_operation fo;

    // 字段增加变量结构体  
    std::vector<field_increase_store_t> vt_field_increase_store_t;
};


// 向表中插入数据运行时信息  
struct insert_table_runtime_info_t {

    // 文件是否打开 默认为false  
    bool is_file_open;

    // 文件操作类指针  
    file_operation fo;

    // 行号  
    int line_no;

    // 文件是否关闭 默认为false 到达文件结束则置为true  
    bool is_end_of_file;

    // 表数据文件开始表中的各个字段名称  
    std::vector<std::string> vt_table_fields_name;

    // 起始时间  
    volatile uint64_t           start_time;

    // path 列表中 索引位置 仅仅是针对 文件才有效   
    size_t index_in_file_path;

    ////////////////////////////////////////////////////////////////////
    // 是否使用导入文件还是产生数据  true : 使用文件  false 产生数据  
    // true : fo 才有可能是被使用 否则不可以被使用  
    bool is_use_file;

    // 在线程内这个是否是带头大哥  如果是的话 is_use_file = true fo 才有用  
    bool is_head_in_thread;

    // 其他跟随者需要直到带头大哥在vector中的索引位置  带头大哥指向自己  
    size_t head_index_in_thread;

    // 最后一个小弟的索引位置的索引位置 由他负责清理数据  
    size_t tail_index_in_thread;

    // 表名字 指的是自己的表名字  
    std::string table_name;

    // 字段列表(包括time) 逗号分割  
    std::string fields;

    // 数据集合 (a, b, c) (d, e, f) 这种形式 可能有一个或者多个   
    std::string datas;

    // 组织sql语句 insert into + table_name + ( + fileds + ) values +  datas 即使一条完整语句  
    // 用于产生数据的vector 带头大哥拥有就可以了  
    std::vector <struct field_increase_store_t> vt_field_increase_store_t;

    // 目前行数 插入表后将line_count 清空一次  
    //int line_count;

    // 是否到达整个插入结束标志 如果设置为true 后续执行全部忽略  
    //bool is_set_end_of_file_or_time_end;

    // 是否允许清除数据了   true 清除数据  false 不允许清除数据  
    //bool is_clear_data;

};


struct test_table_file_info_t;

// 给nane source 用的结构体  
struct nane_source_param_t {
    // 表名前缀  
    std::string table_lead;

    //表模板文件信息   
    struct test_table_file_info_t * ttfi;

};

// 给dir source 用的结构体  
struct dir_source_param_t {
    // 表名前缀  
    std::string table_lead;

    //表模板文件信息   
    struct test_table_file_info_t* ttfi;

    // 目录路径列表  
    std::vector<std::string> vt_dirs;

    // 分隔符号  
    std::string sep;
};

// 表名前缀和表名结构体  
struct table_lead_and_table_name_t {
    // 表名前缀  
    std::string table_lead;

    // 表名全称（包括了表名前缀  
    std::string table_name;
};


// 表名前缀和表数据路径结构体  
struct table_lead_and_table_path_t {
    // 表名前缀  
    std::string table_lead;

    // 表数据路径  
    std::string table_path;
};


//表下字段信息   
struct test_tb_field_info_t {
    // 字段索引  从0开始  
    int index;

    // 字段名字 time FIELD_0 FIELD_1
    std::string name;

    // 字段类型 int bigint bool float double varchar timestamp  
    std::string type;

    /// 字段长度  1, 4, 8, 32 ...
    int len;

    // 字段类型  TSDB_DATATYPE_BOOL TSDB_DATATYPE_INT 等  
    enum tsdb_datatype_t datatype;

    // 字段是否允许为空 默认为空  "user_name varchar(20) NOT NULL,  //不允许为空 ; user_Info varchar(100) NULL   //允许为空" 
    bool is_null;
};



//表文件信息   
struct test_table_file_info_t {
    // 索引从0开始  
    int index;

    // 表名前缀 "process_" "location_"
    std::string table_lead;

    // 主键字段名字  
    std::string primary_key_field_name;


    // 指示表名后缀关联的字段名字  
    std::string table_tail_field_name;

    // 表名后缀列表 允许为空 也可以这样 [1, 3] 指的是 1, 2, 3  
    std::vector<std::string> vt_table_tail;

    // 表字段信息文件路径  
    std::string field_list_file;
    
    // 字段列表数组  
    std::vector<struct test_tb_field_info_t> vt_test_tb_field_info_t;
};


#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////
//
//<private>
// InitializeCriticalSection, DeleteCriticalSection,
// EnterCriticalSection, TryEnterCriticalSection, LeaveCriticalSection
// for Non Windows OS
//
// pthread_mutex_init, pthread_mutex_destroy,
// pthread_mutex_lock, pthread_mutex_trylock, pthread_mutex_unlock
// for Windows OS
//
// Lockable, ScopeLock for C++
//</private>
//

#if defined( _WIN32 ) || defined( _WIN64 )

    //<private>
    // Windows
    //</private>

    typedef struct pthread_mutex_t
    {
        CRITICAL_SECTION    lock;

        unsigned char       is_init;

    } pthread_mutex_t;

    //<private>
    /**
     * @function pthread_mutex_init
     * @brief 
     * @param[in] pthread_mutex_t * p:
     * @param[in] void * zero: not used, must be zero
     */
    //</private>
    static inline
    int pthread_mutex_init( pthread_mutex_t * p, void * zero )
    {
        InitializeCriticalSection( & p->lock );
        p->is_init = 1;
        return 0;
    }

    //<private>
    /**
     * @function pthread_mutex_destroy
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    //</private>
    static inline
    void pthread_mutex_destroy( pthread_mutex_t * p )
    {
        DeleteCriticalSection( & p->lock );
        p->is_init = 0;
    }

    //<private>
    /**
     * @function pthread_mutex_lock
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    //</private>
    static inline
    void pthread_mutex_lock( pthread_mutex_t * p )
    {
        if ( ! p->is_init ) {
            pthread_mutex_init( p, NULL );
        }

        EnterCriticalSection( & p->lock );
    }

    //<private>
    /**
     * @function pthread_mutex_trylock
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    //</private>
    static inline
    int pthread_mutex_trylock( pthread_mutex_t * p )
    {
        if ( ! p->is_init ) {
            pthread_mutex_init( p, NULL );
        }

        return TryEnterCriticalSection( & p->lock ) ? 0 : EBUSY;
    }

    //<private>
    /**
     * @function pthread_mutex_unlock
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    //</private>
    static inline
    void pthread_mutex_unlock( pthread_mutex_t * p )
    {
        LeaveCriticalSection( & p->lock );
    }

#else  // linux  
    

    //<private>
    // Non Windows
    //</private>

    typedef pthread_mutex_t CRITICAL_SECTION;

    //<private>
    /**
     * @function InitializeCriticalSection
     * @brief 
     * @param[out] CRITICAL_SECTION * p: 
     */
    //</private>
    static inline
    void InitializeCriticalSection( CRITICAL_SECTION * p )
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init( & attr );
        pthread_mutexattr_settype( & attr, PTHREAD_MUTEX_RECURSIVE_NP );
        pthread_mutex_init( p, & attr );
        pthread_mutexattr_destroy( & attr );
    }

    //<private>
    /**
     * @function DeleteCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    //</private>
    static inline
    void DeleteCriticalSection( CRITICAL_SECTION * p )
    {
        pthread_mutex_destroy( p );
    }

    //<private>
    /**
     * @function EnterCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    //</private>
    static inline
    void EnterCriticalSection( CRITICAL_SECTION * p )
    {
        pthread_mutex_lock( p );
    }

    //<private>
    /**
     * @function TryEnterCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    //</private>
    static inline
    BOOL TryEnterCriticalSection( CRITICAL_SECTION * p )
    {
        int r = pthread_mutex_trylock( p );
        return (0 == r) ? TRUE : FALSE;
    }

    //<private>
    /**
     * @function LeaveCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    //</private>
    static inline
    void LeaveCriticalSection( CRITICAL_SECTION * p )
    {
        pthread_mutex_unlock( p );
    }

#endif

#ifdef __cplusplus
}
#endif

struct lockable_t
{
public:
    lockable_t()
    {
#if defined( _WIN32 ) || defined( _WIN64 )
        pthread_mutex_init(&m_lock, NULL);
#elif defined(__FreeBSD__) || defined(__APPLE__)
        pthread_mutex_init(&m_lock, NULL);
#else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&m_lock, &attr);
        pthread_mutexattr_destroy(&attr);
#endif
    }

    ~lockable_t()
    {
        pthread_mutex_destroy(&m_lock);
    }

    pthread_mutex_t* get()
    {
        return &m_lock;
    }

    inline void lock()
    {
        pthread_mutex_lock(&m_lock);
    }

    inline bool try_lock()
    {
        int r = pthread_mutex_trylock(&m_lock);
        if (0 == r) {
            return true;
        }
        return false;
    }

    inline bool try_lock(const char* file, int line)
    {
        return try_lock();
    }

    inline bool unlock()
    {
        pthread_mutex_unlock(&m_lock);
        return true;
    }

    inline void lock(const char* file, int line)
    {
        lock();
    }

private:
    pthread_mutex_t m_lock;

private:
    // disable
    lockable_t(const lockable_t&);
    const lockable_t& operator = (const lockable_t&);
};

struct scope_lock_t
{
public:

#if DEBUG_LOCK
    explicit scope_lock_t(lockable_t& lock, const char* file, int line)
        : m_lock(lock)
    {
        m_lock.lock(file, line);
    }
#else
    explicit scope_lock_t(lockable_t& lock, const char*, int) : m_lock(lock) { m_lock.lock(); }

    explicit scope_lock_t(lockable_t& lock) : m_lock(lock) { m_lock.lock(); }
#endif
    ~scope_lock_t() { m_lock.unlock(); }

private:
    lockable_t& m_lock;

private:
    // disable
    scope_lock_t();
    scope_lock_t(const scope_lock_t&);
    const scope_lock_t& operator = (const scope_lock_t&);
};



/**
 * @brief  解析字段列表文件  
 * @param[in]  const char *field_list_file                                           文件路径  
 * @param[out] std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t     字段列表数组  
 * @return  0 成功 其他 失败  
 */
int parse_field_list_file(
    const char* field_list_file,
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t);



/**
 * @brief  解析表配置文件  
 * @param[in]  const char *table_conf_file                                           表配置文件路径  
 * @param[out] std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t 表信息数组  
 *             key : 表名前缀 value : struct test_table_file_info_t
 * @return  0 成功 其他 失败  
 */
int parse_table_conf_file(
    const char* table_conf_file,
    std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t);


/**
 * @brief  将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
 * @param[in] std::map < std::string,  std::vector<std::string> > & map_vt_table_head_data_path 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @param[in]  std::vector<struct table_lead_and_table_name_t> &vt_table_lead_and_table_name_t  表名信息vector形式  
 * @return  0 成功 其他 失败  
 */
int convert_table_conf_map_to_table_vector(
    const std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t,
    std::vector<struct table_lead_and_table_name_t> &vt_table_lead_and_table_name_t);



/**
 * @brief  将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
 * @param[in] const std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @param[out] std::vector<struct test_table_file_info_t*> &vt_test_table_file_info_t  表信息vector形式  
 *             注意  test_table_file_info_t * 指向输入的map 的指针 后续不用考虑释放问题  
 * @return  0 成功 其他 失败  
 */
int convert_table_conf_map_to_table_vector_ex(
    const std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t,
    std::vector<struct test_table_file_info_t*> &vt_test_table_file_info_t);

/**
 * @brief  解析表数据文件  
 * @param[in]  const char* table_data_conf_file                                           表数据配置文件路径  
 * @param[out] std::map <std::string,  CONFIG_DATA_PATHS_SEP_T> & map_vt_table_head_data_path 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @return  0 成功 其他 失败  
 */
int parse_table_data_conf_file(
    const char* table_data_conf_file,
    std::map <std::string, CONFIG_DATA_PATHS_SEP_T> & map_vt_table_head_data_path);


/**
 * @brief  将表数据信息转化vector格式 目的是为了便于将表数据发送到各个线程中  
 * @param[in] std::map < std::string,  CONFIG_DATA_PATHS_SEP_T > & map_vt_table_head_data_path 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @param[out]  std::vector<struct table_lead_and_table_path_t> &vt_table_lead_and_table_name_t  表名信息vector形式  
 * @return  0 成功 其他 失败  
 */
int convert_table_data_map_to_table_vector(
    const std::map < std::string,  CONFIG_DATA_PATHS_SEP_T > & map_vt_table_head_data_path,
    std::vector<struct table_lead_and_table_path_t> &vt_table_lead_and_table_path_t);


/**
 * @brief  从csv 文件中获取一行格式好的数据  
 * @param[in]   file_operation& fo                      文件操作类型 必须以读方式打开  
 * @param[in]   const char* sep                         csv 分隔符号  
 * @param[out]  std::vector<tsdb_str>& vt_data,         格式好的数据  
 * @param[out]  std::vector<BOOL>& vt_data_is_string    指示是否是字符串类型  
 * @param[out]  int *line_no                            行号  
 * @return  0 成功 ENODATA 表示已经读到文件尾部了 其他情况失败  
 */
int get_format_line_from_csv_file(
    file_operation& fo,
    const char* sep,
    std::vector<tsdb_str>& vt_data,
    std::vector<BOOL>& vt_data_is_string,
    int *line_no);

/**
 * @brief  为int初始化数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[in]  int def_value                               初始值  
 * @param[in]  float fstep                                 步长  
 * @return   0 成功 其他 错误  
 */
int init_data_for_int(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int def_value, float fstep);

/**
 * @brief  为int64初始化数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[in]  int64_t def_value                           初始值  
 * @param[in]  float fstep                                 步长  
 * @return   0 成功 其他 错误  
 */
int init_data_for_int64(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int64_t def_value, float fstep);

/**
 * @brief  为int64初始化数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[in]  float def_value                             初始值  
 * @param[in]  float fstep                                 步长  
 * @return   0 成功 其他 错误  
 */
int init_data_for_float(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, float def_value, float fstep);


/**
 * @brief  为int产生数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] int &value                                  返回值  
 * @return   0 成功 其他 错误  
 */
int generate_data_for_int(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int &value);

/**
 * @brief  为int64产生数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] int64_t &value                              返回值  
 * @return   0 成功 其他 错误  
 */
int generate_data_for_int64(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int64_t &value);


/**
 * @brief  为float产生数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] float &value                                返回值  
 * @return   0 成功 其他 错误  
 */
int generate_data_for_float(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, float &value);


/**
 * @brief  为string产生数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] std::string &value                          返回值  
 * @return   0 成功 其他 错误  
 */
int generate_data_for_string(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, std::string &value);

/**
 * @brief  对集合初始化默认数据  
 * @param[in]  std::vector <struct field_increase_store_t> &vt_field_increase_store_t          字段增加变量结构体  
 * @param[in]  std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t               字段详细信息集合  
 * @param[in]  uint64_t start_time                         起始时间对时间戳字段有用 其他字段可忽略  
 * @param[in]  float fstep                                 步长 各个字段设置相同  
 * @return   0 成功 其他 错误  
 */
int init_data_for_batch_by_default(
    std::vector <struct field_increase_store_t> &vt_field_increase_store_t, 
    std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t,
    uint64_t start_time, 
    float fstep);



/**
 * @brief  根据各个字段产生一行field  
 * @param[in]  std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t               字段详细信息集合  
 * @param[in]  int start_index                                                                 从那个index 开始  
 * @param[in]  const char* sep                                                                 分隔符号  
 * @param[out] std::string &line                                                               获取的一行数据  
 * @return   0 成功 其他 错误  
 */
int generate_field_for_one_line(std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t, int start_index, const char* sep, std::string &line);


/**
 * @brief  根据各个字段产生一行数据  
 * @param[in]  std::vector <struct field_increase_store_t> &vt_field_increase_store_t          字段增加变量结构体  
 * @param[in]  std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t               字段详细信息集合  
 * @param[in]  int start_index                                                                 从那个index 开始  
 * @param[in]  const char* sep                                                                 分隔符号  
 * @param[out] std::string &line                                                               获取的一行数据  
 * @return   0 成功 其他 错误  
 */
int generate_data_for_one_line(std::vector <struct field_increase_store_t>& vt_field_increase_store_t,
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t, int start_index, const char* sep, std::string &line);

/**
 * @brief  为字符串和时间戳增加单引号 如果之前已经加了的话 则啥也不做   
 * @param[out] std::string& value         需要将 ts 加上 并增加单引号   
 * @param[in]  tsdb_str& ts               字段详细信息集合  
 * @param[in]  enum tsdb_datatype_t  datatype     数据类型 仅仅为字符串和时间戳  
 * @param[in]  datatype,   int max_len    字符串允许的最大长度  
 * @param[in]  bool is_convert_time       是否转化时间(仅仅针对时间戳类型) 目前是针对taos对时间戳兼容不好做的处理    
 * @return   0 成功 其他 错误  
 */
int add_add_single_quotes_for_string_and_timestamp(std::string& value, tsdb_str& ts, enum tsdb_datatype_t datatype, int max_len, bool is_convert_time);

/**
 * @brief  打印当前路径  
 * @param[in]  无  
 * @return   忽略返回值  
 */
int print_current_path();

/**
 * @brief   判断字符串是否全部为数字   
 * @param[in]  const std::string& str     输入的字符串   
 * @param[in]  tsdb_str& ts               字段详细信息集合  
 * @param[in]  enum tsdb_datatype_t  datatype     数据类型 仅仅为字符串和时间戳  
 * @return   true 全部都是数字 false 不全是数字  
 */
bool is_digit_all(const std::string& str);

/**
 * @brief   整理bool值尽可能兼容boolean值 能够处理"true" 'true' "false" 'false' 0, 1 之类  
 * @param[out] std::string& value         处理为true 或者false   
 * @param[in]  tsdb_str& ts               字段详细信息集合  
 * @param[in]  enum tsdb_datatype_t  datatype     数据类型 仅仅为字符串和时间戳  
 * @return   0 成功 其他 错误  
 */
int deal_with_for_boolean(std::string& value, tsdb_str& ts, enum tsdb_datatype_t datatype);


/**
 * @brief  整理分隔符号 目前支持 "\t" " " "\n" "\r" "\'" "\"" "," 符号  
 * @param[in]  const char *sep            从命令行获取到的分隔符号  
 * @param[out] std::string &sep_after     返回处理后的分隔符号  
 * @return   0 成功 其他 错误  
 */
int tidy_separate_symbol(const char *sep, std::string &sep_after);

/**
 * @brief  去除头尾字符仅仅是清理一次  
 * @param[in]  const std::string &source  原始字符串  
 * @param[in]  const char sep             分隔符号  
 * @param[out] std::string &target        返回处理后的字符串  
 * @return   0 成功 其他 错误  
 */
int trim_first_and_last_char_only_once(const std::string& source, const char sep, std::string& target);

} // namespace wide

} // namespace rtdb

#endif

#ifndef _tsdb_tools_h_
#define _tsdb_tools_h_

#include "dpr_stdinc.h"
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
    #include <sys/types.h>
    #include <dirent.h> // opendir
#endif
#ifdef __cplusplus
    #include <vector>
    #include <string>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// forward declare
        struct tsdb_fifo_t;
        struct tsdb_fifo_t;
typedef struct tsdb_fifo_t          tsdb_fifo_t;
        struct tsdb_tools_t;
typedef struct tsdb_tools_t         tsdb_tools_t;
#ifndef _fclass_include_dpr_json_h_
        struct dpr_json_t;
typedef struct dpr_json_t           dpr_json_t;
        struct dpr_json_memory_t;
typedef struct dpr_json_memory_t    dpr_json_memory_t;
        struct dpr_json_ctxt_t;
typedef struct dpr_json_ctxt_t      dpr_json_ctxt_t;
#endif // #ifndef _fclass_include_dpr_json_h_

#if ! defined(_fclass_base_string_h_) && ! defined(_GROCKET_INCLUDE_GRLIB_H_) 

    #define CHARSET_UNKNOWN     0
    #define CHARSET_GBK         1
    #define CHARSET_UTF8        2
    #define CHARSET_UCS2LE      3
    #define CHARSET_UCS2BE      4
    #define CHARSET_BIG5        5
    #define CHARSET_EUCJP       6
    #define CHARSET_SJIS        7
    #define CHARSET_EUCKR       8
    #define CHARSET_ISO1        9
    #define CHARSET_WIN1        10
    #define CHARSET_WIN2        11

#endif // #ifndef _fclass_base_string_h_

#ifndef LOG_LEVEL_T_DEFINED
#define LOG_LEVEL_T_DEFINED
typedef enum
{
    // enable all log
    LOG_ALL          = 0,
    // enable debug or higher log
    LOG_DBG        = 1,
    // enable info or higher log
    LOG_INF         = 2,
    // enable warning or higher log
    LOG_WRN      = 3,
    // enable error or higher log
    LOG_ERR        = 4,
    // enable fatal or higher log
    LOG_FATAL        = 5,
    // disable log
    LOG_NONE         = 6,
    
    LOG_LEVEL_COUNT  = 7
} log_level_t;

typedef void ( * grocket_log_t )(
    const char *    file,
    int             line,
    const char *    func,
    log_level_t     level,
    va_list         valist,
    const char **   fmt
);

typedef void ( * grocket_log_huge_t )(
    const char *    file,
    int             line,
    const char *    func,
    log_level_t     level,
    bool            is_user_log,
    const char *    data,
    size_t          data_len
);
#endif // #ifndef LOG_LEVEL_T_DEFINED

#ifndef _DPR_PTHREAD_T_DEFINED_
    #define _DPR_PTHREAD_T_DEFINED_ 1
    #if defined( _WIN32 ) || defined( _WIN64 )
        typedef HANDLE      pthread_t;
    #else
        //typedef pthread_t   thread_t;
    #endif
#endif // #ifndef _DPR_PTHREAD_T_DEFINED_

#if defined( _WIN32 ) || defined( _WIN64 )
    #if ! defined( DPR_DIRENT_H )
        #define DPR_DIRENT_H    1

        // dirent structure - used by the dirent.h directory iteration functions
        typedef struct dirent
        {
            // d_reclen is not compatible with linux
            //unsigned short	    d_reclen;	// Length of name in d_name.
            char *              d_name;		// File name.
            unsigned char       d_type;     // 4->directory; 8->file
        } dirent;

        // DIR structure - used by the dirent.h directory iteration functions
        typedef struct DIR
        {
	        // disk transfer area for this dir
            WIN32_FIND_DATAA    dd_dta;
	        // dirent struct to return from dir (NOTE: this makes this thread
	        // safe as long as only one thread uses a particular DIR struct at
	        // a time)
	        struct dirent       dd_dir;
	        // _findnext handle
	        HANDLE              dd_handle;
	        //
            //   * Status of search:
	        //   0 = not started yet (next entry to read is first entry)
	        //  -1 = off the end
	        //   positive = 0 based index of next entry
	        //
	        int             dd_stat;
	        // given path for dir with search pattern (struct is extended)
	        char            dd_name[ MAX_PATH ];
        } DIR;
    #endif // #ifndef DPR_DIRENT_H
#endif

#ifndef TSDB_STR_DEFINED
#define TSDB_STR_DEFINED
/**
 * @brief point to string, also working in binary data  
 */
typedef struct tsdb_str
{
    /// point to string  
    const char *    ptr;

    /// string length by charactor, not including \0  
    int             len;

#ifdef __cplusplus
    tsdb_str() : ptr( NULL ), len( 0 ) {}
    tsdb_str( const char * ptr2, int len2 ) : ptr( ptr2 ), len( len2 ) {}
    tsdb_str(const tsdb_str & rhd) : ptr( rhd.ptr ), len( rhd.len ) {}
    const tsdb_str & operator = (const tsdb_str & rhd) { if ( this != & rhd ) { ptr = rhd.ptr; len = rhd.len; } return * this; }
    void clear() { ptr = NULL; len = 0; }
#endif

} tsdb_str;
#endif // #ifndef TSDB_STR_DEFINED

// task interface var C interface
struct tsdb_task_t
{
    void          ( * kill_me )( struct tsdb_task_t * task );

    //<private>
    // must call kill_me before exit function
    //</private>
    void          ( * proc_and_kill_me )( struct tsdb_task_t * task );
};

#ifdef __cplusplus

    namespace tsdb_inner
    {
        static void tsdb_task_cpp_kill_me( struct tsdb_task_t * task );
        static void tsdb_task_cpp_proc_and_kill_me( struct tsdb_task_t * task );
    } // namespace tsdb_inner

    // task interface var CPP interface
    class tsdb_task_cpp_t : public tsdb_task_t
    {
    public:
        tsdb_task_cpp_t() {
            kill_me             = tsdb_inner::tsdb_task_cpp_kill_me;
            proc_and_kill_me    = tsdb_inner::tsdb_task_cpp_proc_and_kill_me;
        }
        virtual ~tsdb_task_cpp_t() {}
        virtual void process_and_release() = 0;
        virtual void release() = 0;
    private:
        // disable
        tsdb_task_cpp_t(const tsdb_task_cpp_t &rhd);
        const tsdb_task_cpp_t & operator = (const tsdb_task_cpp_t &);
    };

    namespace tsdb_inner
    {
        static void tsdb_task_cpp_kill_me( struct tsdb_task_t * task )
        {
            if ( task ) {
                ((tsdb_task_cpp_t*)task)->release();
            }
        }

        static void tsdb_task_cpp_proc_and_kill_me( struct tsdb_task_t * task )
        {
            if ( task ) {
                ((tsdb_task_cpp_t*)task)->process_and_release();
            }
        }
    } // namespace tsdb_inner

#endif // #ifdef __cplusplus

struct tsdb_fifo_t
{
    void          ( * kill_me )( tsdb_fifo_t * fifo );

    int           ( * start )  ( tsdb_fifo_t * fifo );
    int           ( * stop )   ( tsdb_fifo_t * fifo );

    /*
     * @param[in] tsdb_fifo_t * fifo    valid fifo object pointer
     * @param[in] tsdb_task_t * task    valid task pointer.
     *                                  If the call fails, the task will be killed via task->kill_me( task )
     * @return int error code, 0 if OK.
     */
    int           ( * push )   ( tsdb_fifo_t * fifo, tsdb_task_t * task );

    int           ( * size )   ( tsdb_fifo_t * fifo );
};

typedef enum dpr_json_type_t
{
    DPR_JSON_FALSE          = 0,

    DPR_JSON_TRUE           = 1,

    DPR_JSON_NULL           = 2,

    DPR_JSON_NUMBER         = 3,

    DPR_JSON_STRING         = 4,

    DPR_JSON_ARRAY          = 5,

    DPR_JSON_OBJECT         = 6,

    DPR_JSON_IS_REFERENCE   = 256

} dpr_json_type_t;

struct dpr_json_t
{
    dpr_json_t *        next;
    dpr_json_t *        prev;
    dpr_json_t *        child;

    dpr_json_type_t     type;
    char *              key;

    int64_t             valueint;
    double              valuedouble;
    char *              valuestr;
    int                 valuestr_len;
};

#ifndef _fclass_include_dpr_json_h_

struct dpr_json_memory_t
{
    void * ( * node_malloc )( size_t sz );
    void   ( * node_free   )( void * ptr );

    void * ( * malloc )( size_t sz );
    void   ( * free   )( void * ptr );
};

struct dpr_json_ctxt_t
{
    dpr_json_memory_t   memory;

    const char *        strerr;
};

#endif // #ifndef _fclass_include_dpr_json_h_

#ifndef DPR_PARSER_DEFINED
    #define DPR_PARSER_DEFINED  1
    typedef struct parser2_t
    {
        const char *    begin;
        const char *    end;
        const char *    cur;
        int             charset;

    } parser2_t;
#endif

struct tsdb_tools_t
{
    ///////////////////////////////////////////////////////////////////
    // Tool  

    /**
     * @brief Get log path  
     * @return const char * The path, if invalid, returns ""
     */
    const char * ( * get_log_path )();

    /**
     * @brief ???????????? time_t ?????????????????????  
     * @param[in] int64_t v     time_t ?????????  
     * @param[out] int * year   ???  
     * @param[out] int * year   ???  
     * @param[out] int * year   ???  
     * @param[out] int * year   ???  
     * @param[out] int * year   ???  
     * @param[out] int * year   ???  
     * @return BOOL ??????????????????  
     */
    BOOL ( * time_info )(
        int64_t                         v,
        int *                           year,
        int *                           month,
        int *                           day,
        int *                           hour,
        int *                           minute,
        int *                           second
    );

    /**
     * @brief ????????????????????????????????? time_t
     * @param[in] int  year   ???  
     * @param[in] int  year   ???  
     * @param[in] int  year   ???  
     * @param[in] int  year   ???  
     * @param[in] int  year   ???  
     * @param[in] int  year   ???  
     * @return int64_t ?????? time_t ???  
     */
    int64_t ( * time_make )(
        int                             year,
        int                             month,
        int                             day,
        int                             hour,
        int                             minute,
        int                             second
    );

    /**
     * @brief ?????????????????????????????? time_t
     * @param[in] const char * str  ?????????  
     * @param[in] int str_len       ???????????????  
     * @return int64_t ?????? time_t ???  
     */
    int64_t ( * time_from_str )(
        const char *                    str,
        int                             str_len
    );

    /**
     * @brief ??????????????? time_t ????????????????????????????????????  
     * @param[in] int64_t v         time_t ?????????  
     * @param[in] char * str        ????????????????????????????????????  
     * @param[in,out] int * str_len ????????????????????????????????????????????????????????????????????????  
     * @return BOOL ????????????  
     */
    BOOL ( * time_to_str )(
        int64_t                         v,
        char *                          str,
        int *                           str_len
    );

    /**
     * @brief ????????????????????????1970?????????????????????  
     * @return uint64_t result ??????????????????  
     */
    uint64_t ( * datetime_now )();

    /**
     * @brief ??????????????????????????????????????????1970?????????????????????  
     * @param[in] int  year         ???  
     * @param[in] int  month        ???  
     * @param[in] int  day          ???  
     * @param[in] int  hour         ???  
     * @param[in] int  minute       ???  
     * @param[in] int  second       ???  
     * @param[in] int  ms           ??????  
     * @return uint64_t ????????????0????????????  
     */
    uint64_t ( * datetime_make )(
        int                             year,
        int                             month,
        int                             day,
        int                             hour,
        int                             minute,
        int                             second,
        int                             ms
    );

    /**
     * @brief ???????????????1970??????????????????????????????????????????  
     * @param[in] uint64_t ticks    ?????????  
     * @param[out] int* year        ???  
     * @param[out] int* month       ???  
     * @param[out] int* day         ???  
     * @param[out] int* hour        ???  
     * @param[out] int* minute      ???  
     * @param[out] int* second      ???  
     * @param[out] int* ms          ??????  
     * @return BOOL ??????????????????  
     */
    BOOL ( * datetime_info )(
        uint64_t                        ticks,
        int *                           year,
        int *                           month,
        int *                           day,
        int *                           hour,
        int *                           minute,
        int *                           second,
        int *                           ms
    );

    /**
     * @brief ?????????????????????????????????1970???????????????????????????  
     * @param[in] const char * str  ?????????  
     * @param[in] int str_len       ???????????????  
     * @return uint64_t ?????????1970??????????????????????????????  
     */
    uint64_t ( * datetime_from_str )(
        const char *                    str,
        int                             str_len
    );

    /**
     * @brief ???????????????1970???????????????????????????????????????????????????????????????  
     * @param[in] uint64_t v        ?????????  
     * @param[in] char * str        ????????????????????????????????????  
     * @param[in,out] int * str_len ????????????????????????????????????????????????????????????????????????  
     * @return BOOL ????????????  
     */
    BOOL ( * datetime_to_str )(
        uint64_t                        v,
        char *                          str,
        int *                           str_len
    );

    /**
     * @brief ???????????????????????????  
     * @param[in] int argc           ????????????  
     * @param[in] char ** argv       ??????  
     * @param[in] const char * key   ???????????? key
     * @param[out] const char ** val ???????????????  
     * @param[out] size_t * val_len  ??????????????????????????????????????????????????????????????????  
     * @return BOOL ????????????  
     */
    BOOL ( * find_argv )(
        int             argc,
        char **         argv,
        const char *    key,
        const char **   value,
        size_t *        value_len
    );

    /**
     * @brief ?????????????????????????????????  
     * @param[in] int argc           ????????????  
     * @param[in] char ** argv       ??????  
     * @param[in] const char * key   ???????????? key
     * @param[out] int * value       int??????  
     * @return BOOL ????????????  
     */
    BOOL ( * find_argv_int )(
        int             argc,
        char **         argv,
        const char *    key,
        int *           value
    );

    /**
     * @brief ?????????????????????64 bit????????????  
     * @param[in] int argc           ????????????  
     * @param[in] char ** argv       ??????  
     * @param[in] const char * key   ???????????? key
     * @param[out] int64 * value     int64??????  
     * @return BOOL ????????????  
     */
    BOOL ( * find_argv_int64 )(
        int             argc,
        char **         argv,
        const char *    key,
        int64_t *       value
    );

    /**
     * @brief ????????????????????????????????????  
     * @param[in] int argc           ????????????  
     * @param[in] char ** argv       ??????  
     * @param[in] const char * key   ???????????? key
     * @param[out] BOOL def_val      ?????????  
     * @return BOOL ????????????  
     */
    BOOL ( * find_argv_bool )(
        int             argc,
        char **         argv,
        const char *    key,
        BOOL            def_val
    );

    /**
     * @brief ?????????????????????????????????  
     * @param[in] int argc           ????????????  
     * @param[in] char ** argv       ??????  
     * @param[in] const char * key   ???????????? key
     * @param[out] uint64_t * value  value  
     * @return BOOL ????????????  
     */
    BOOL ( * find_argv_datetime )(
        int             argc,
        char **         argv,
        const char *    key,
        uint64_t *      value
    );

    /**
     * @brief ????????????????????????????????????????????????  
     * @return unsigned long ?????????    
     */
    unsigned long ( * get_tick_count )();
    uint64_t      ( * get_tick_count_us )();

    /**
     * @brief ???????????????????????????  
     * @return void  
     */
    void ( * sleep_ms )( unsigned int ms );

    /**
     * @brief ????????????  
     * @param[in] const char * path  ?????????????????????  
     * @return BOOL ????????????  
     */
    BOOL ( * make_dir )( const char * dir );

    /**
     * @brief ????????????  
     * @param[in] const char * path  ?????????????????????  
     * @return BOOL ????????????  
     */
    BOOL ( * del_file )( const char * path );

    /**
     * @brief ????????????  
     * @param[in] const char * path  ?????????????????????  
     * @return BOOL ????????????  
     */
    BOOL ( * del_dir )( const char * dir );

    /**
     * @brief ?????????????????????????????????  
     * @param[in] const char * path  
     * @return BOOL ???????????????  
     */
    BOOL ( * is_file )( const char * path );

    /**
     * @brief ?????????????????????????????????  
     * @param[in] const char * path  
     * @return BOOL ???????????????  
     */
    BOOL ( * is_dir )( const char * path );

    /**
     * @brief ????????????????????????????????????  
     * @param[in] const char * path  
     * @return BOOL ??????????????????  
     */
    BOOL ( * is_empty_dir )( const char * path );

    /**
     * @brief ?????????????????????????????????  
     * @param[in] const char * path  ????????????  
     * @param[out] void * data       ?????????????????????????????????????????????NULL?????? data_len ????????????????????????  
     * @param[in/out] int64_t * data_len ?????? data ???????????????????????????????????????????????????  
     * @return int error code  EINVAL   - ???????????????  
     *                         ENOENT   - ???????????????  
     *                         EIO      - I/O????????????  
     *                         EMSGSIZE - ????????????????????????data_len ???????????????????????????  
     */
    int ( * load_file )( const char * path, void * data, int64_t * data_len );

    /**
     * @brief ??????????????????????????????  
     * @param[in] const char * s ???????????????????????????  
     * @param[in/out] int * len  ??????????????????????????????????????????????????????  
     * @return const char * ?????? s ???????????????????????????????????????????????????  
     */
    const char * ( * str_trim_const )(
        const char *    s,
        int *           len
    );

    /**
     * @brief ??????????????????????????????  
     * @param[in/out] char * s ???????????????????????????  
     * @param[in/out] int * len  ??????????????????????????????????????????????????????  
     * @return const char * ?????? s ????????????????????????????????????????????????  
     */
    char * ( * str_trim )(
        char *          s,
        int *           len
    );

    /**
     * @brief ???????????????????????????  
     * @param[in] const char * path
     * @return int64_t ???????????????????????? (int64_t)-1
     */
    int64_t ( * get_file_size )( const char * path );

    /**
     * @brief ????????????????????????????????????????????????????????????  
     * @param[in/out] char * path
     * @return void
     */
    void ( * path_to_os )( char * path );

    int ( * get_exe_path )( char * path, int  path_len );
    int ( * get_dll_path )( void *  func, char * path, int  path_len );
    int ( * get_exe_dir )( char * dir, int dir_len, BOOL add_sep );
    int ( * get_cur_dir )( char * dir, int dir_len, BOOL add_sep );

    /**
     * @brief ??? double ?????????????????????  
     * @param[in] double v          ????????? double
     * @param[out] char * result    ???????????????  
     * @param[in] size_t result_max ?????????????????????????????? '\0'  
     * @return ?????? result
     */
    const char * ( * double_to_str )( double  v, char * result, int result_max );

    /**
     * @brief ?????????????????????????????????  
     * @param[in] const char * str  ?????????  
     * @param[in] int str_len       ???????????????  
     * @return ???????????????????????????  
     */
    BOOL ( * is_all_digit )(
        const char *    str,
        int             str_len
    );

    BOOL ( * to_array )(
        char *          src,
        const char *    sep,
        char **         result,
        int *           result_count
    );

    BOOL ( * to_const_array )(
        const char *    src,
        int             src_len,
        const char *    sep,
        int             sep_len,
        tsdb_str *      result,
        int *           result_count
    );

    /**
     * @brief ???????????????????????? int ???????????????  
     * @param[in] const char * src   ?????????  
     * @param[in] int src_len        ???????????????  
     * @param[in] const char * sep   ?????????  
     * @param[in] int sep_len        ???????????????  
     * @param[out]int * result       ?????? int ?????????????????????????????????NULL?????? result_len ????????????????????????????????????TRUE  
     * @param[in/out] int*result_len ?????? result ??????????????????????????????????????????????????????  
     *                               ?????????????????????????????????????????????????????????????????????????????????????????????????????????FALSE  
     * @return BOOL ????????????  
     */
    BOOL ( * to_int_array )(
        const char *    src,
        int             src_len,
        const char *    sep,
        int             sep_len,
        int *           result,
        int *           result_len
    );

    int ( * log_open )( const char * log_dir, const char * log_name, log_level_t level );

    void ( * log_set_level )( log_level_t level );

    void ( * log_enable_stdout )( BOOL enable );
    void ( * log_enable_stderr )( BOOL enable );

    void ( * log_write )(
        const char *    file,
        int             line,
        const char *    func,
        log_level_t     level,
        BOOL            is_user_log,
        const char *    fmt,
        ...
    );

    void ( * log_write_huge )(
        const char *    file,
        int             line,
        const char *    func,
        log_level_t     level,
        BOOL            is_user_log,
        const char *    data,
        size_t          data_len
    );

    int ( * fp_read )(
        FILE *          fp,
        void *          data,
        int             data_len
    );

    BOOL ( * fp_write )(
        FILE *          fp,
        const void *    data,
        size_t          data_len
    );

    void ( * simple_encrypt )( void *buf, int buf_len, uint32_t passwd );
    void ( * simple_decrypt )( void *buf, int buf_len, uint32_t passwd );

    BOOL ( * base64_encode )(
        const void *    input,
        int             input_len,
        int             crlf_len,
        char *          output,
        int *           output_len
    );

    BOOL ( * base64_decode )(
        const char *    input,
        int             input_len,
        void *          output,
        int *           output_len
    );

    int ( * zcompress )(
        const void *    data,
        int             data_len,
        void *          zdata,
        int *           zdata_len
    );

    int ( * zdecompress )(
        const void *    zdata,
        int             zdata_len,
        void *          data,
        int *           data_len
    );

    BOOL ( * socket_str_2_addr_v4 )(
        const char * str,
        struct sockaddr_in * addr
    );

    int ( * socket_tcp_v4 )();
    int ( * socket_udp_v4 )();

    /**
     * @brief close socket
     * @param[in] SOCKET sock: socket fd that will be close
     */
    int ( * socket_close )(
	    int             sock
    );

    /**
     * @brief Is TCP use delay algorithem? 
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] bool isNoDelay: is it no delay? if true,
     *                            single send call will be fire a real send.
     * @return bool: is OK?
     */
    BOOL ( * socket_get_tcp_no_delay )(
	    int             sock,
	    BOOL *          isNoDelay
    );

    BOOL ( * socket_set_tcp_no_delay )(
	    int             sock,
	    BOOL            isNoDelay
    );

    /**
     * @brief Is TCP use KeepAlive?
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] bool isKeepAlive: is it KeepAlive
     * @return bool: is OK?
     */
    BOOL ( * socket_set_keep_alive )(
	    int             sock,
	    BOOL            isKeepAlive
    );

    /**
     * @brief set send buffer
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] int bytes: send buffer bytes
     * @return bool: is OK?
     */
    BOOL ( * socket_get_send_buf )(
	    int             sock,
	    int *           bytes
    );

    BOOL ( * socket_set_send_buf )(
	    int             sock,
	    int             bytes
    );

    /**
     * @brief set recv buffer
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] int bytes: recv buffer bytes
     * @return bool: is OK?
     */
    BOOL ( * socket_get_recv_buf )(
	    int             sock,
	    int *           bytes
    );

    BOOL ( * socket_set_recv_buf )(
	    int             sock,
	    int             bytes
    );

    /**
     * @brief set TTL
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] int ttl: TTL
     * @return bool: is OK?
     */
    BOOL ( * socket_set_ttl )(
	    int             sock,
	    int             ttl
    );

    BOOL ( * socket_set_loopback )(
        int             sock,
        BOOL            enable
    );

    BOOL ( * socket_get_linger )(
        int             sock,
        int *           lv
    );

    BOOL ( * socket_set_linger )(
        int             sock,
        int             linger
    );

    /**
     * @brief if last socket call failed, is it because E_INPROGRESS or E_WOULDBLOCK
     * @param[in] SOCKET sock: SOCKET fd
     * @return bool: yes or no
     */
    BOOL ( * socket_is_pending )();

    /**
     * @brief same as socket recv function
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] void * buf: recv buffer
     * @param[in] int bytes: recv buffer bytes
     * @return int: readed bytes, < 0 if failed
     */
    int ( * socket_recv )(
	    int             sock,
	    void *          buf,
	    int             bytes
    );

    /**
     * @brief same as socket send function
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] void * buf: data pointer that will be send
     * @param[in] int bytes: data bytes
     * @return int: sent bytes
     */
    int ( * socket_send )(
	    int             sock,
	    const void *    buf,
	    int             bytes
    );

    BOOL ( * socket_recv_fill )(
        int             sock,
        void *          buf,
        int             bytes,
        int             timeout_ms,
        BOOL *          is_timeout
    );

    /**
     * @brief send all data
     * @param[in] SOCKET sock: SOCKET fd
     * @param[in] void * buf: data pointer that will be send
     * @param[in] int bytes: data bytes
     * @return BOOL: is all sent
     */
    BOOL ( * socket_send_all )(
	    int             sock,
	    const void *    buf,
	    int             bytes,
        BOOL            is_async_socket,
        int             timeout_ms
    );

    void ( * md5 )(
        const void *    src,
        int             src_len,
        void *          result,
        int             result_len
    );

    unsigned long ( * crc32 )(
        unsigned long  crc,
        const void *   buf,
        int            len
    );

    unsigned int ( * hash_bytes )(
        const void *    data,
        unsigned int    bytes,
        unsigned int    seed
    );

    tsdb_fifo_t * ( * fifo_new )();

    int ( * get_cpu_core_count )();

    int ( * charset_convert )(
        int             src_type,
        const void *    src,
        int             src_bytes,
        int             dst_type,
        void *          dst,
        int *           dst_bytes
    );

    BOOL ( * bytes_to_hex )(
        const void *    bytes,
        int             length,
        char *          result,
        int             result_length,
        BOOL            write_end_char
    );

    BOOL ( * hex_to_bytes )(
        const char *    hex,
        int             length,
        void *          result,
        int             result_length
    );

    int ( * json_init )( dpr_json_ctxt_t * ctxt, dpr_json_memory_t * ptr );

    dpr_json_t * ( * json_parse )( dpr_json_ctxt_t * ctxt, const char * str, int str_len );

    void ( * json_delete )( dpr_json_ctxt_t * ctxt, dpr_json_t * node );

    //<private>
    /**
     * @brief create a new thread
     * @param[out] thread thread create successed, then write thread_id to thread param.
     * @param[in] start_routine thread rountine
     * @param[in] arg thread rountine parameter
     * @param[in] priority thread priority: -3, -2, -1, 0, 1, 2, 3
     * @return int return true if successed; otherwise return false
     */
    //</private>
    BOOL ( * thread_start )(
        pthread_t * thread,
        void *      (*start_routine)(void*),
        void *      arg,
        int         priority
    );

    //<private>
    /**
     * @brief wait thread exit
     * @param[in] threadid
     */
    //</private>
    void ( * thread_join )(
        pthread_t * thread
    );

    //<private>
    /**
     * @function dll_open
     * @brief open a dynamic library
     * @param[in] const TCHAR * path: path for dynamic library
     * @return dll_t: not NULL if successed, NULL if failed
     * @code
     
    dll_t h = dll_open( "./MyLib" );
    .\MyLib.dll on windows, ./libMyLib.dylib on iOS, ./libMyLib.so on Android
    * @endcode
    */
    //</private>
    dll_t ( * dll_open )(
        const char * path
    );

    dll_t ( * dll_open_absolute )(
        const char * path
    );

    //<private>
    /**
     * @brief close a dynamic library
     * @param[in] dll_t: dynamic library handle
     */
    //</private>
    void ( * dll_close )(
        dll_t h
    );

    //<private>
    /**
     * @brief query a function, that export function
     * @param[in] dll_t: dynamic library handle
     * @param[in] const char * func_name: function name 
     */
    //</private>
    void * ( * dll_symbol )(
        dll_t h,
        const char * func_name
    );

    //
    // Returns a pointer to a DIR structure appropriately filled in to begin
    // searching a directory.
    //
    DIR * ( * opendir )( const char * filespec );

    //
    // Return a pointer to a dirent structure filled with the information on the
    // next entry in the directory.
    //
    struct dirent* ( * readdir )( DIR * dir );

    //
    // Frees up resources allocated by opendir.
    //
    int	( * closedir )( DIR * dir );

    BOOL ( * parser_open_charset )(
        parser2_t *      parser,
        const void *    ptr,
        int             len,
        int             charset
    );

    BOOL ( * parser_open )(
        parser2_t *      parser,
        const void *    ptr,
        int             len
    );

    BOOL ( * parser_end )(
        parser2_t *      parser
    );

    char ( * parser_peek )(
        parser2_t *      parser
    );

    char ( * parser_read )(
        parser2_t *      parser
    );

    int ( * parser_read_charset )(
        parser2_t *      parser,
        char *          result,
        int *           result_len
    );

    const char * ( * parser_read_charset_ptr )(
        parser2_t *      parser,
        int *           result_len
    );

    void ( * parser_back )(
        parser2_t *      parser
    );

    int ( * parser_back_bytes )(
        parser2_t *      parser,
        int              bytes
    );

    int ( * parser_ignore_spaces )(
        parser2_t *      parser
    );

    int ( * parser_ignore_spaces_tail )(
        parser2_t *      parser
    );

    int ( * parser_ignore_db_spaces_tail )(
        parser2_t *  parser
    );

    int ( * parser_ignore_to )(
        parser2_t *          parser,
        const char *        stop_chars
    );

    int ( * parser_escape_char )(
        parser2_t *      parser,
        char *          result
    );

    int ( * parser_read_string )(
        parser2_t *      parser,
        BOOL            translate_escape_char,
        char *          result,
        int *           result_len
    );

    int ( * parser_read_whole_string )(
        parser2_t *      parser,
        BOOL            translate_escape_char,
        char *          result,
        int *           result_len
    );

    const char * ( * parser_read_string_ptr )(
        parser2_t *      parser,
        int *           result_len
    );

    int ( * parser_html_escape_char )(
        parser2_t *      parser,
        char *          result,
        int *           result_len
    );

    int ( * parser_read_html_string )(
        parser2_t *      parser,
        BOOL            entity_decode,
        char *          result,
        int *           result_len
    );

    int ( * parser_read_whole_html_string )(
        parser2_t *      parser,
        BOOL            entity_decode,
        char *          result,
        int *           result_len
    );

    const char * ( * parser_read_html_string_ptr )(
        parser2_t *      parser,
        int *           result_len
    );

    int ( * parser_read_to )(
        parser2_t *          parser,
        const char *        stop_chars,
        BOOL                enable_escape,
        char *              result,
        int *               result_len,
        BOOL                ignore_stop_char
    );

    const char * ( * parser_read_ptr_to )(
        parser2_t *          parser,
        const char *        stop_chars,
        int *               result_len,
        BOOL                ignore_stop_char
    );

    int ( * parser_read_word )(
        parser2_t *          parser,
        BOOL                enable_escape,
        char *              result,
        int *               result_len
    );

    const char * ( * parser_read_word_ptr )(
        parser2_t *          parser,
        int *               result_len
    );

    BOOL ( * parser_read_last_word )(
        parser2_t *          parser,
        BOOL                enable_escape,
        char *              result,
        int *               result_len
    );

    int ( * parser_read_alpha )(
        parser2_t *          parser,
        BOOL                enable_escape,
        char *              result,
        int *               result_len
    );

    int ( * parser_read_int )(
        parser2_t *      parser,
        int *           result
    );

    int ( * parser_read_number )(
        parser2_t *      parser,
        char *          result,
        int *           result_len
    );

    time_t ( * parser_read_datetime_rfc867 )(
        parser2_t *      parser
    );

    const tsdb_str * ( * get_sentence_sep_list2 )(
        int             charset,
        int *           count
    );

    const char * ( * parser_read_sentence_ptr )(
        parser2_t *      parser,
        int *           result_len,
        const char **   sep
    );

    /**
     * @brief read CSV line data into result
     * @param[in] const char * line      A CSV line data
     * @param[in] int line_len           data len, not including '\0'
     * @param[in] const char * sep       separator of column
     * @param[out] const_str * data      result data array, if this parameter is NULL, then data_count return real field_count
     * @param[out] BOOL * data_is_string 
     * @param[in/out] int * data_count   input max of data count, return real data count
     * @param[return] int error code. 
     */
    int ( * csv_line_to_array2 )(
        char *          line,
        int             line_len,
        const char *    sep,
        tsdb_str *      data,
        BOOL *          data_is_string,
        int *           data_count
    );

};

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////
//
//
//

#ifndef _fclass_base_atomic_h_
#define _fclass_base_atomic_h_

#include "dpr_stdinc.h"

#if defined( _WIN32 ) || defined( _WIN64 )
    #include <winsock2.h>
    #include <windows.h>
#elif defined( __APPLE__ )
    //zouyueming 2013-10-26 19:58 since OS X 10.9, Apple add __header_always_inline
    // function into OSAtomic.h header. What fucken means?
    #ifdef __header_always_inline
        #undef __header_always_inline
    #endif
    #define __header_always_inline  static inline
    #include <libkern/OSAtomic.h>
#elif defined( __FreeBSD__ )
    #include <sys/types.h>
    #include <machine/atomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// @return old value
// atomic_t atomic_fetch_add( int v, atomic_t * dst )
// atomic_t atomic_fetch_set( int v, atomic_t * dst )

#ifndef ATOMIC_T_DEFINED
    #define ATOMIC_T_DEFINED    1
    #if defined( __linux )
        // atomic_t is 32 bit in linux
        typedef volatile int                atomic_t;
    #elif defined( __APPLE__ )
        // gr_atomic_t is 32 bit in Apple OS X
        typedef volatile int32_t            atomic_t;
    #elif defined( __FreeBSD__ )
        // gr_atomic_t is 32 bit in Apple OS X
        typedef volatile u_long             atomic_t;
    #elif defined( _WIN32 ) || defined( _WIN64 )
        // gr_atomic_t is 32 bit in Windows. long is 4 bytes on windows.
        typedef long volatile               atomic_t;
    #else
        #error unknown platform
    #endif
    #ifndef ATOMIC_T_LEN
        #define ATOMIC_T_LEN                4
    #endif
#endif // #ifndef ATOMIC_T_DEFINED


#if defined( __linux )
    #if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 1) || (__GNUC__ == 4 && __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ >= 2)
        // GCC including __sync_fetch_and_add function since 4.1.2
        // __ANDROID__ reach this way.
        static inline atomic_t atomic_fetch_add( int v, atomic_t * dst )
        {
            return __sync_fetch_and_add( dst, v );
        }

        static inline bool atomic_compare_set_int( int from, int to, atomic_t * dst )
        {
            return __sync_bool_compare_and_swap( dst, from, to );
        }

        static inline bool atomic_compare_set_ptr( void * from, void * to, void * volatile * dst )
        {
            return __sync_bool_compare_and_swap( dst, from, to );
        }

/*        static __inline atomic_t atomic_fetch_set( int v, atomic_t * dst )
        {
            //<private>
            //TODO: ??????????????? Centos 7.3(kernel 3.10.0) ????????????????????? CentOS 6.5(kernel 2.6.32-431) ???????????????  
            // ????????? Centos 6.5 ???????????? __atomic_exchange_n ???????????? __ATOMIC_SEQ_CST ????????????  
            //</private>
            return (int)__atomic_exchange_n( (int*)dst, (int)v, __ATOMIC_SEQ_CST );
        }
*/
    #elif defined( __x86_64 )
        // 64 bit X86 CPU
        static inline atomic_t atomic_fetch_add( int v, atomic_t * dst )
        {
            __asm__ __volatile__(
                " lock ; xaddl %0, %1; "
                : "+r" (v) : "m" (*dst) : "cc", "memory"
            );
            return v;
        }

    #else
        // 32 bit X86 CPU
        static inline atomic_t atomic_fetch_add( int v, atomic_t * dst)
        {
            asm volatile(
                "movl        %0,%%eax;"
                "movl        %1,%%ecx;"
                "lock xadd   %%eax,(%%ecx);"
                ::"m"(v), "m"(dst)
            );
            return v;
        }
    #endif

#elif defined( __APPLE__ )

    static inline atomic_t atomic_fetch_add( int v, atomic_t * dst )
    {
        return OSAtomicAdd32( (int32_t)(v), (atomic_t *)(dst) ) - v;
    }

#elif defined( __FreeBSD__ )

    static inline u_int inner_atomic_fetchsub_int( atomic_t * dst, u_int v )
    {
        __asm __volatile(
        "   lock ; "
        "   xsubl %0, %1 ; "
        "# atomic_fetchsub_int"
        :  "+r" (v),        // 0 (result)
           "=m" (*dst)      // 1
        :  "m"  (*dst));    // 2
        return v;
    }

    static inline atomic_t atomic_fetch_add( int v, atomic_t * dst )
    {
        //TODO: freebsd
        return ( v >= 0 )
            ? (atomic_t)atomic_fetchadd_int( dst, (u_int)(v) )
            : (atomic_t)inner_atomic_fetchsub_int( dst, (u_int)(v) );
    }

#elif defined( _WIN32 ) || defined( _WIN64 )

    static __inline atomic_t atomic_fetch_add( int v, atomic_t * dst )
    {
        return InterlockedExchangeAdd( (long volatile *)(dst), (long)(v) );
    }

    static __inline atomic_t atomic_fetch_set( int v, atomic_t * dst )
    {
        return InterlockedExchange( (long volatile *)dst, (long)v );
    }

    static __inline bool atomic_compare_set_int( int from, int to, atomic_t * dst )
    {
        LONG r = InterlockedCompareExchange( (long volatile*)(dst), (long)to, (long)from );
        return ( (int)r == from ) ? true : false;
    }

    static __inline bool atomic_compare_set_ptr( void * from, void * to, void * volatile * dst )
    {
        void * r = InterlockedCompareExchangePointer( dst, to, from );
        return ( r == from ) ? true : false;
    }

#else
    #error unknown platform
#endif

#ifdef __cplusplus
}
#endif

#endif // ! #ifndef _fclass_base_atomic_h_

//
//
//
///////////////////////////////////////////////////////////////////////
//
// ??????????????????????????????????????????????????? dpr_lock.h ????????????  
//

#ifndef _fclass_base_lock_h_
#define _fclass_base_lock_h_

#include "dpr_stdinc.h"

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

#else

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

#define DEBUG_LOCK  0

//<private>
/**
 * @class lockable_t
 * @brief 
 */
//</private>
struct lockable_t
{
public:
    lockable_t()
    {
#if defined( _WIN32 ) || defined( _WIN64 )
        pthread_mutex_init( & m_lock, NULL );
#elif defined(__FreeBSD__) || defined(__APPLE__)
        pthread_mutex_init( & m_lock, NULL );
#else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init( & attr );
        pthread_mutexattr_settype( & attr, PTHREAD_MUTEX_RECURSIVE_NP );
        pthread_mutex_init( & m_lock, & attr );
        pthread_mutexattr_destroy( & attr );
#endif
    }

    ~lockable_t()
    {
        pthread_mutex_destroy( & m_lock );
    }

    pthread_mutex_t * get()
    {
        return & m_lock;
    }

    inline void lock()
    {
        pthread_mutex_lock( & m_lock );
    }

    inline bool try_lock()
    {
        int r = pthread_mutex_trylock( & m_lock );
        if ( 0 == r ) {
            return true;
        }
        return false;
    }

    inline bool try_lock( const char * file, int line )
    {
        return try_lock();
    }

    inline bool unlock()
    {
        pthread_mutex_unlock( & m_lock );
        return true;
    }

    inline void lock( const char * file, int line )
    {
        lock();
    }

private:
    pthread_mutex_t m_lock;

private:
    // disable
    lockable_t(const lockable_t &);
    const lockable_t & operator = (const lockable_t &);
};

//<private>
/**
 * @class scope_lock_t
 * @brief 
 */
//</private>
struct scope_lock_t
{
public:

#if DEBUG_LOCK
    explicit scope_lock_t( lockable_t & lock, const char * file, int line )
        : m_lock( lock )
    { m_lock.lock( file, line ); }
#else
    explicit scope_lock_t( lockable_t & lock, const char *, int ) : m_lock( lock ) { m_lock.lock(); }

    explicit scope_lock_t( lockable_t & lock ) : m_lock( lock ) { m_lock.lock(); }
#endif
    ~scope_lock_t() { m_lock.unlock(); }

private:
    lockable_t & m_lock;

private:
    // disable
    scope_lock_t();
    scope_lock_t(const scope_lock_t &);
    const scope_lock_t & operator = (const scope_lock_t &);
};

struct scope_lock2_t
{
public:
    explicit scope_lock2_t( pthread_mutex_t & lock ) : m_lock( lock ) { pthread_mutex_lock( & m_lock ); }
    ~scope_lock2_t() { pthread_mutex_unlock( & m_lock ); }

private:
    pthread_mutex_t & m_lock;

private:
    // disable
    scope_lock2_t();
    scope_lock2_t(const scope_lock2_t &);
    const scope_lock2_t & operator = (const scope_lock2_t &);
};

#endif // #ifdef __cplusplus

///////////////////////////////////////////////////////////////////////
//
//<private>
// rwlockable_t, scope_rlock_t, scope_wlock_t
//</private>
//

#ifdef __cplusplus

#if defined( _WIN32 ) || defined( _WIN64 )

struct rwlockable_t
{
public:
    rwlockable_t() : m_nReaders(0), m_nWriters(0)
    {
        m_hDataEvent = CreateEvent(
            NULL,    // no security attributes
            FALSE,   // Auto reset event
            FALSE,   // initially set to non signaled state
            NULL);   // un named event
        InitializeCriticalSection( & m_WriteLock );
    }

    ~rwlockable_t()
    {
        DeleteCriticalSection( & m_WriteLock );
        CloseHandle( m_hDataEvent );
    }

    void rlock() const
    {
        //<private>
        // ???????????????,???  
        //</private>
        while( m_nReaders > 0 ) {
            WaitForSingleObject( m_hDataEvent, 50 );
        }

        InterlockedIncrement( & m_nReaders );
    }

    void runlock() const
    {
        long n = InterlockedDecrement( & m_nReaders );
        if ( 0 == n ) {
            SetEvent( m_hDataEvent );
        }
    }

    void wlock() const
    {
        //<private>
        // ??????????????????,???  
        //</private>
        while ( m_nReaders > 0 || m_nWriters > 0 ) {
            WaitForSingleObject( m_hDataEvent, 50 );
        }

        InterlockedIncrement( & m_nWriters );

        EnterCriticalSection( & m_WriteLock );
    }

    void wunlock() const
    {
        LeaveCriticalSection( & m_WriteLock );

        long n = InterlockedDecrement( & m_nWriters );

        if ( 0 == n ) {
            SetEvent( m_hDataEvent );
        }
    }

private:
    mutable volatile long       m_nReaders;
    mutable volatile long       m_nWriters;
    mutable CRITICAL_SECTION    m_WriteLock;
    mutable HANDLE              m_hDataEvent;

private:
    // disable
    rwlockable_t(const rwlockable_t &);
    const rwlockable_t & operator = (const rwlockable_t &);
};

#elif defined( __ANDROID__ )

struct rwlockable_t
{
public:
    rwlockable_t() { pthread_mutex_init( & m_lock, NULL ); }
    ~rwlockable_t() { pthread_mutex_destroy( & m_lock ); }

    void rlock() const { pthread_mutex_lock( & m_lock ); }
    void runlock() const { pthread_mutex_unlock( & m_lock ); }

    void wlock() const { pthread_mutex_lock( & m_lock ); }
    void wunlock() const { pthread_mutex_unlock( & m_lock ); }

private:
    mutable pthread_mutex_t m_lock;

private:
    // disable
    rwlockable_t(const rwlockable_t &);
    const rwlockable_t & operator = (const rwlockable_t &);
};
#else

struct rwlockable_t
{
public:
    rwlockable_t() { pthread_rwlock_init( & m_lock, NULL ); }
    ~rwlockable_t() { pthread_rwlock_destroy( & m_lock ); }

    void rlock() const { pthread_rwlock_rdlock( & m_lock ); }
    void runlock() const { pthread_rwlock_unlock( & m_lock ); }

    void wlock() const { pthread_rwlock_wrlock( & m_lock ); }
    void wunlock() const { pthread_rwlock_unlock( & m_lock ); }

private:
    mutable pthread_rwlock_t m_lock;

private:
    // disable
    rwlockable_t(const rwlockable_t &);
    const rwlockable_t & operator = (const rwlockable_t &);
};

#endif

struct scope_rlock_t
{
public:
    explicit scope_rlock_t( rwlockable_t & lock ) : m_lock( lock ) {
        m_lock.rlock();
    }

    ~scope_rlock_t() {
        m_lock.runlock();
    }

private:
    rwlockable_t & m_lock;

private:
    // disable
    scope_rlock_t();
    scope_rlock_t(const scope_rlock_t &);
    const scope_rlock_t & operator = (const scope_rlock_t &);
};

struct scope_wlock_t
{
public:
    explicit scope_wlock_t( rwlockable_t & lock ) : m_lock( lock ) {
        m_lock.wlock();
    }

    ~scope_wlock_t() {
        m_lock.wunlock();
    }

private:
    rwlockable_t & m_lock;

private:
    // disable
    scope_wlock_t();
    scope_wlock_t(const scope_wlock_t &);
    const scope_wlock_t & operator = (const scope_wlock_t &);
};

#endif // #ifdef __cplusplus

//
// rwlockable_t, scope_rlock_t, scope_wlock_t
//
///////////////////////////////////////////////////////////////////////

#endif // #ifndef _fclass_base_lock_h_

#ifndef SCOPE_LOCK
    #define SCOPE_LOCK_ENABLE_LOG	0

    //<private>
    // ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????  
    //</private>
    #if SCOPE_LOCK_ENABLE_LOG
        #include "dpr/dpr_log.h"
        #define SCOPE_LOCK( lockp )         \
            LOG_INFO( "LOCK COMING" );      \
            scope_lock_t lock( (*lockp) )
    #else
        #define SCOPE_LOCK( lockp )         scope_lock_t lock( (*lockp) )
    #endif
#endif

//
//
///////////////////////////////////////////////////////////////////////

#endif // #ifndef _tsdb_tools_h_

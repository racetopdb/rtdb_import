#ifndef _tsdb_client_v3_h_
#define _tsdb_client_v3_h_

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
    #include <dlfcn.h>      // dlopen
    #include <sys/stat.h>   // stat
#endif

//<private>
/// version of current interface, even change compability, we must increment this version
//</private>
#define TSDB_V3_VERSION                 ((uint64_t)202207281444)
//<private>
/// version of lowest interface, for compability
//</private>
#define TSDB_V3_VERSION_LOW             ((uint64_t)202207281444)
//<private>
/// server version
//</private>
#define TSDB_SERVER_VERSION             ((uint64_t)202207281444)

#define TSDB_FIELD_NAME_LEN             128
#define TSDB_TABLE_NAME_LEN             128
#define TSDB_DATABASE_NAME_LEN          128

        struct tsdb_tools_t;
typedef struct tsdb_tools_t tsdb_tools_t;

#include <stdint.h>     // memset
#include <string.h>     // memset
#include <stdlib.h>     // free

// S_PATH_SEP
// S_PATH_SEP_C
#if ! defined( S_PATH_SEP_DEFINED )
    #define S_PATH_SEP_DEFINED  1
    #if defined( _WIN32 ) || defined( _WIN64 )
        #define S_PATH_SEP_C    '\\'
        #define S_PATH_SEP      "\\"
    #else
        #define S_PATH_SEP_C    '/'
        #define S_PATH_SEP      "/"
    #endif
#endif

#if ! defined( LIKELY_DEFINED )
    #define LIKELY_DEFINED 1
    #if defined( _WIN32 ) || defined( _WIN64 )
        #define likely(x)   (x)
        #define unlikely(x) (x)
    #else
        #define likely(x)   __builtin_expect((x),1)
        #define unlikely(x) __builtin_expect((x),0)
    #endif
#endif

#if ! defined( BYTE_DEFINED )
    #define BYTE_DEFINED    1
    //<private>
    // byte_t
    //</private>
    typedef unsigned char           byte_t;
#endif

#if ! defined( BOOL_DEFINED )
    #if defined( _WIN32 ) || defined( _WIN64 )

        #ifdef _WIN32_WINNT
            #undef _WIN32_WINNT
        #endif
        #define _WIN32_WINNT 0x0501 // CreateWaitableTimer, LPFN_CONNECTEX

        //<private>
        // compatible after MFC headers
        //</private>
        #if ! defined( __AFX_H__ )
            #include <winsock2.h>
            #include <ws2tcpip.h>
            #include <windows.h>
        #endif

        #define BOOL_DEFINED    1
    #else
        #define BOOL_DEFINED    1
        typedef int BOOL;
    #endif
#endif // #if ! defined( BOOL_DEFINED )

#if ! defined( HINSTANCE_DEFINED )
    #define HINSTANCE_DEFINED   1
    #if defined(WIN32) || defined(WIN64)
	    typedef	HINSTANCE	    dll_t;
    #else
	    typedef void *		    dll_t;

        #define HINSTANCE       dll_t
        #define LoadLibraryA    dll_open
        #define GetProcAddress  dll_symbol
        #define FreeLibrary     dll_close
    #endif
#endif // #if ! defined( HINSTANCE_DEFINED )

#if ! defined( S_EXP )
    //<private>
    // S_EXP
    // S_IMP
    //</private>
    #if ( defined(_MSC_VER) || defined(__CYGWIN__) || (defined(__HP_aCC) && defined(__HP_WINDLL) ) )
        #define S_EXP               __declspec(dllexport)
        #define S_IMP               __declspec(dllimport)
    #elif defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550)
        #define S_EXP               __global
        #define S_IMP
    #else
        #define S_EXP
        #define S_IMP
    #endif
#endif // #if ! defined( S_EXP )

#ifndef TSDB_STR_DEFINED
#define TSDB_STR_DEFINED
/**
 * @brief string pointer
 */
typedef struct tsdb_str
{
    const char *    ptr;
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

#ifdef __cplusplus
extern "C" {
#endif

// forward declare
struct tsdb_v3_t;
struct tsdb_v3_iterator_t;
struct tsdb_v3_reader_t;
struct tsdb_v3_field_t;
struct tsdb_v3_reader_t;
struct tsdb_datatype_info_t;
struct tsdb_reader_t;

typedef struct tsdb_v3_t                tsdb_v3_t;
typedef struct tsdb_v3_iterator_t       tsdb_v3_iterator_t;
typedef struct tsdb_v3_reader_t         tsdb_v3_reader_t;
typedef struct tsdb_v3_field_t          tsdb_v3_field_t;
typedef struct tsdb_v3_reader_t         tsdb_v3_reader_t;
typedef struct tsdb_datatype_info_t     tsdb_datatype_info_t;
typedef tsdb_v3_t                       RTDB;
typedef tsdb_v3_reader_t                RTDB_RES;
typedef tsdb_v3_field_t                 RTDB_FIELD;

/**
 * @brief Protocol type of network transmission
 */
typedef enum tsdb_protocol_type_t
{
    /// Unknown protocol
    TSDB_PROTOCOL_UNKNOWN       = 0,

    /// TCP full duplex
    TSDB_PROTOCOL_TCP           = 1,

    /// UDP full duplex
    TSDB_PROTOCOL_UDP           = 2

#define TSDB_PROTOCOL_MIN       0
#define TSDB_PROTOCOL_MAX       2

} tsdb_protocol_type_t;

/**
 * @brief union for value
 */
union tsdb_value_t
{
    byte_t          bool_val;
    int             int_val;
    int64_t         int64_val;
    float           float_val;
    double          double_val;
    int             tag_val;
};

/**
 * @brief data type
 //<private>
 * @warning 如果本枚举有增减，则对应的 tsdb_table_server.cpp 文件的 g_get_last_line_first_key 要做相应的修改。  
 //<private>
 */
enum tsdb_datatype_t
{
    /// unknown data type
    TSDB_DATATYPE_UNKNOWN       = 0,

    /// boolean. 1 byte
    TSDB_DATATYPE_BOOL          = 1,

    /// int. 4 bytes
    TSDB_DATATYPE_INT           = 2,

    /// int 64 bit. 8 bytes
    TSDB_DATATYPE_INT64         = 3,

    /// float. 4 bytes
    TSDB_DATATYPE_FLOAT         = 4,

    /// double. 8 bytes
    TSDB_DATATYPE_DOUBLE        = 5,

    /// binary
    TSDB_DATATYPE_BINARY        = 6,

    /// string
    TSDB_DATATYPE_STRING        = 7,

    /// datetime ms
    TSDB_DATATYPE_DATETIME_MS   = 8,

    /// void *
    TSDB_DATATYPE_POINTER       = 9,

    /// tag string
    TSDB_DATATYPE_TAG_STRING    = 10,

#define TSDB_DATATYPE_MIN_VAL  1
#define TSDB_DATATYPE_MAX_VAL  9

};

struct  tsdb_datatype_info_t
{
    /// data type
    tsdb_datatype_t             data_type;

    /// data type names
    const char * *              names;

    /// Is the field enable NULL
    unsigned int                enable_null : 1;

    /// The minimum length for the user
    /// For fixed length data type, this value same with max_length.
    unsigned int                min_length  : 31;

    /// The maximum length for the user
    /// For fixed length data type, this value same with min_length.
    unsigned int                max_length;

    /// The actually bytes in the data line.
    /// 0 indicate this is a variant length data type.
    unsigned int                storage_bytes;

    /// length for show
    unsigned int                show_length;

    struct tsdb_type_funcs_t *  funcs;
};

#define TSDB_V3_FIELD_INDEX_INVALID     0xFFFF

struct tsdb_v3_field_t
{
#ifdef __cplusplus
    tsdb_v3_field_t()
        : name( NULL )
        , field_index( TSDB_V3_FIELD_INDEX_INVALID )
        , data_type( TSDB_DATATYPE_UNKNOWN )
        , unique( 0 )
        , has_index( 0 )
        , is_ref( 0 )
        , is_null( 0 )
        , length( 0 )
        , field_id( 0 )
        , real_length( 0 )
    {
        _reserved[ 0 ] = 0;
        _reserved[ 1 ] = 0;
    }
#endif

    //<private>
    /// field name
    //</private>
    const char *    name;

    /// field index
    uint16_t        field_index;

    /// tsdb_datatype_t
    byte_t          data_type       : 4;

    /// is unique
    byte_t          unique          : 1;

    /// has index
    byte_t          has_index       : 1;

    /// is reference
    byte_t          is_ref          : 1;

    /// NULL or NOT NULL
    byte_t          is_null         : 1;

    /// max width, including '\0'
    byte_t          length;

    /// field_id (field_cbid)
    uint32_t        field_id;

    /// max width in current recordset, NOT including '\0'
    byte_t          real_length;

    /// field name length, NOT including '\0'
    byte_t          name_length;

    char            _reserved[ 2 ];
};

/**
 * @brief tsdb_v3_reader_t's iterator object
 */
struct tsdb_v3_iterator_t
{
    /**
     * @brief kill current iterator object
     * @param[in] tsdb_v3_iterator_t *  res
     * @see tsdb_v3_t::query()
     */
    void ( * kill_me )( tsdb_v3_iterator_t * res );

    /**
     * @brief Get current iterator's table
     * @param[in] tsdb_v3_iterator_t *  res
     * @return tsdb_v3_reader_t * 
     * @see tsdb_v3_t::query()
     */
    tsdb_v3_reader_t * ( * parent )( tsdb_v3_iterator_t * res );

    /**
     * @brief is the iterator reach the end of recordset
     * @param[in] tsdb_v3_iterator_t *  res
     * @return BOOL
     */
    BOOL ( * is_eof )( tsdb_v3_iterator_t * res );

    /**
     * @brief Gets next row in current search result.
     *        If the current query statement does not return a result data,
     *        this function returns ENODATA
     * @param[in] tsdb_v3_iterator_t *  res
     * @return int   0: row readded; ENODATA: no more data
     * @see tsdb_v3_t::query()
     */
    int ( * next )( tsdb_v3_iterator_t * res );

    /**
     * @brief Query the records of the specified row_index.
     *        If the current query statement does not return a result data,
     *        this function returns ENODATA
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[in] uint64_t              row_index
     * @param[in,optional] tsdb_v3_reader_t *    table
     * @return int   0: row readded; ENODATA: no more data
     * @see tsdb_v3_t::query()
     */
    int ( * set )( tsdb_v3_iterator_t * res, uint64_t row_index, tsdb_v3_reader_t * table );

    /**
     * @brief reset the cursor. After calling this function, 
     *        you can use cursor_next function to visit all data.
     * @param[in] tsdb_v3_iterator_t *  res
     * @return int 
     * @see tsdb_v3_t::query()
     */
    int ( * reset )( tsdb_v3_iterator_t * res );

    /**
     * @brief Gets the field count in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns 0
     * @param[in] tsdb_v3_iterator_t *  res
     * @return uint32_t field count in result data set.
     * @see tsdb_v3_t::query()
     */
    uint32_t ( * field_count )( tsdb_v3_iterator_t * res );

    /**
     * @brief Gets special field in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns NULL
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[int] uint32_t field_index    field index
     * @return RTDB_FIELD * field
     * @see tsdb_v3_t::query()
     */
    tsdb_v3_field_t * ( * field_get )( tsdb_v3_iterator_t * res, uint32_t field_index );

    /**
     * @brief find special field by name in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns NULL
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] const char * name
     * @return RTDB_FIELD * field
     * @see tsdb_v3_t::query()
     */
    tsdb_v3_field_t * ( * field_find )( tsdb_v3_iterator_t * res, const char * name );

    const tsdb_datatype_info_t * ( * field_get_datatype )( tsdb_v3_iterator_t * res, uint32_t field_index );
    const tsdb_datatype_info_t * ( * field_find_datatype )( tsdb_v3_iterator_t * res, const char * name );

    /**
     * @brief Gets the length of the specified field to display data
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[in] const char * name
     * @return RTDB_FIELD * field
     * @see tsdb_v3_t::query()
     */
    int ( * field_show_length )( tsdb_v3_iterator_t * res, uint32_t field_index );

    /**
     * @brief Gets the row count in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns 0
     * @param[in] tsdb_v3_iterator_t *  res
     * @return uint64_t row count in result data set.
     * @see tsdb_v3_t::query()
     */
    uint64_t ( * row_count )( tsdb_v3_iterator_t * res );

    /**
     * @brief Gets the data of the specified field of the current record.
     *        For the convenience of display, the returned data fills the remaining space with spaces.
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[in] uint32_t field_index
     * @param[out] int * len      Return data length
     * @return const char *       value
     * @see tsdb_v3_t::query()
     */
    const char * ( * get_str_aligned )( tsdb_v3_iterator_t * res, uint32_t field_index, int * len );

    /**
     * @brief Gets the specified field name.
     *        For the convenience of display, the returned data fills the remaining space with spaces.
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[in] uint32_t field_index
     * @param[out] int * len      Return field name length
     * @return const char * field_name
     * @see tsdb_v3_t::query()
     */
    const char * ( * get_field_aligned )( tsdb_v3_iterator_t * res, uint32_t field_index, int * len );

    /**
     * @brief Checks whether the value of the specified field in the current row is NULL
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[in] uint32_t field_index
     * @return BOOL  return TRUE if the value is NULL
     * @see tsdb_v3_t::query()
     */
    BOOL         ( * is_null       )( tsdb_v3_iterator_t * res, uint32_t field_index );
    BOOL         ( * is_null_s     )( tsdb_v3_iterator_t * res, const char * field );

    /**
     * @brief get the bool field value in the current row, return NULL if NULL
     * @param[in] tsdb_v3_iterator_t *  res
     * @param[in] uint32_t field_index
     * @return byte_t *  return point of byte_t, 1 indicate TRUE, and 0 indicate FALSE
     * @see tsdb_v3_t::query()
     */
    byte_t *     ( * get_bool       )( tsdb_v3_iterator_t * res, uint32_t field_index );
    int *        ( * get_int        )( tsdb_v3_iterator_t * res, uint32_t field_index );
    int64_t *    ( * get_int64      )( tsdb_v3_iterator_t * res, uint32_t field_index );
    int64_t *    ( * get_datetime_ms)( tsdb_v3_iterator_t * res, uint32_t field_index );
    float *      ( * get_float      )( tsdb_v3_iterator_t * res, uint32_t field_index );
    double *     ( * get_double     )( tsdb_v3_iterator_t * res, uint32_t field_index );
    const char * ( * get_string     )( tsdb_v3_iterator_t * res, uint32_t field_index, uint32_t * length );
    const void * ( * get_pointer    )( tsdb_v3_iterator_t * res, uint32_t field_index );

    byte_t *     ( * get_bool_s         )( tsdb_v3_iterator_t * res, const char * field );
    int *        ( * get_int_s          )( tsdb_v3_iterator_t * res, const char * field );
    int64_t *    ( * get_int64_s        )( tsdb_v3_iterator_t * res, const char * field );
    int64_t *    ( * get_datetime_ms_s  )( tsdb_v3_iterator_t * res, const char * field );
    float *      ( * get_float_s        )( tsdb_v3_iterator_t * res, const char * field );
    double *     ( * get_double_s       )( tsdb_v3_iterator_t * res, const char * field );
    const char * ( * get_string_s       )( tsdb_v3_iterator_t * res, const char * field, uint32_t * length );
    const void * ( * get_pointer_s      )( tsdb_v3_iterator_t * res, const char * field );
};

enum tsdb_qrsp_type_t
{
    //<private>
    // 当前记录集返回的是用户的查询结果，注意根据查询语句决定，查询结果可能有返回记录集，也可能没有返回记录集。  
    //</private>
    TSDB_QRSP_USER_QUERY_RESULT     = 0,

    //<private>
    // 当前记录集返回的是错误信息表，截止到 2022-05-10 08:47 该类型的返回记录集还未实现。  
    //</private>
    TSDB_QRSP_ERROR_INFO            = 1
};

struct tsdb_v3_reader_t
{
    // inner object, caller never use it
    struct tsdb_reader_t *  _inner;

    /**
     * @brief get data type info
     * @param[in] int datatype  TSDB_DATATYPE_*
     * @return const tsdb_datatype_info_t * point to datatype info
     */
    const tsdb_datatype_info_t * ( * datatype_info )( int datatype );
    const tsdb_datatype_info_t * ( * datatype_find )( const char * name, int name_len );

    /**
     * @brief delete the reader object
     * @param[in] tsdb_v3_reader_t * res    the reader object
     * @return const tsdb_datatype_info_t * point to datatype info
     */
    void ( * kill_me )( tsdb_v3_reader_t * res );

    /**
     * @brief Connect to the specified data source through a conn_str.
     *        The "local" table does not implement this function and return ENOSYS
     * @param[in] tsdb_v3_reader_t * res  reader object. got it by @table_new function
     * @param[in] const char * conn_str   connection string
     * @return int error code
     */
    int ( * open )( tsdb_v3_reader_t * res, const char * conn_str );

    /**
     * @brief execute a query, you can call query much time in a tsdb_v3_reader_t
     * @param[in] tsdb_v3_reader_t * res  reader object. got it by @table_new function
     * @param[in] const char * sql
     * @param[in] int          sql_len
     * @return int error code
     */
    int ( * query )(
        tsdb_v3_reader_t *          res,
        const char *                sql,
        int                         sql_len
    );

    /**
     * @brief Gets the field count in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns 0
     * @param[in] tsdb_v3_reader_t *  res
     * @return uint32_t field count in result data set.
     * @see tsdb_v3_t::query()
     */
    uint32_t ( * field_count )( tsdb_v3_reader_t * res );

    /**
     * @brief add a new field. If this function is not implemented, return ENOSYS
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] const char * name       field name, Case dependent
     * @param[in] int datatype            tsdb_datatype_t
     * @param[in] uint32_t length         max length
     * @param[in] BOOL is_null            enable index on this field
     * @param[out] uint32_t *             new field index
     * @return int error code
     * @see tsdb_v3_t::query()
     */
    int ( * field_add )(
        tsdb_v3_reader_t *  res,
        const char *        name,
        int                 datatype,
        uint32_t            length,
        BOOL                is_null,
        uint32_t *          field_index
    );

    /**
     * @brief add a new pointer field. If this function is not implemented, return ENOSYS.
     *        This function just valid in "local" reader.
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] const char * name       field name, Case dependent
     * @param[in] int datatype            tsdb_datatype_t
     * @param[in] uint32_t length         max length
     * @param[in] BOOL is_null            enable index on this field
     * @param[out] uint32_t *             new field index
     * @return int error code.
     * @see tsdb_v3_t::query()
     */
    //<private>
    //TODO: 2021-09-01 22:22 zouyueming: 需要仔细看源码，确认一下为什么 field_add_ref 还要必须正确填写 length 呢？  
    //</private>
    int ( * field_add_ref )(
        tsdb_v3_reader_t *  res,
        const char *        name,
        int                 datatype,
        uint32_t            length,
        BOOL                is_null,
        uint32_t *          field_index
    );

    /**
     * @brief Gets special field in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns NULL
     * @param[in] tsdb_v3_reader_t *  res
     * @param[int] uint32_t field_index    field index
     * @return RTDB_FIELD * field
     * @see tsdb_v3_t::query()
     */
    tsdb_v3_field_t * ( * field_get )( tsdb_v3_reader_t * res, uint32_t field_index );

    /**
     * @brief find special field by name in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns NULL
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] const char * name
     * @return RTDB_FIELD * field
     * @see tsdb_v3_t::query()
     */
    tsdb_v3_field_t * ( * field_find )( tsdb_v3_reader_t * res, const char * name );

    const tsdb_datatype_info_t * ( * field_get_datatype )( tsdb_v3_reader_t * res, uint32_t field_index );
    const tsdb_datatype_info_t * ( * field_find_datatype )( tsdb_v3_reader_t * res, const char * name );

    /**
     * @brief Gets the length of the specified field to display data
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] const char * name
     * @return RTDB_FIELD * field
     * @see tsdb_v3_t::query()
     */
    int ( * field_show_length )( tsdb_v3_reader_t * res, uint32_t field_index );

    /**
     * @brief Gets the row count in the current search result.
     *        If the current query statement does not return a result data,
     *        this function returns 0
     * @param[in] tsdb_v3_reader_t *  res
     * @return uint64_t row count in result data set.
     * @see tsdb_v3_t::query()
     */
    uint64_t ( * row_count )( tsdb_v3_reader_t * res );

    /**
     * @brief create a new iterator
     * @param[in] tsdb_v3_reader_t *  res
     * @return tsdb_v3_iterator_t *
     * @see tsdb_v3_t::query()
     */
    tsdb_v3_iterator_t * ( * cursor_new )( tsdb_v3_reader_t * res );

    /**
     * @brief Gets next row in current search result.
     *        If the current query statement does not return a result data,
     *        this function returns ENODATA
     * @param[in] tsdb_v3_reader_t *  res
     * @return int   0: row readded; ENODATA: no more data
     * @see tsdb_v3_t::query()
     */
    int ( * cursor_next )( tsdb_v3_reader_t * res );

    int ( * cursor_set )( tsdb_v3_reader_t * res, uint64_t row_index );

    /**
     * @brief reset the cursor. After calling this function, 
     *        you can use cursor_next function to visit all data.
     * @param[in] tsdb_v3_reader_t *  res
     * @return int 
     * @see tsdb_v3_t::query()
     */
    int ( * cursor_reset )( tsdb_v3_reader_t * res );

    /**
     * @brief Gets the data of the specified field of the current record.
     *        For the convenience of display, the returned data fills the remaining space with spaces.
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] uint32_t field_index
     * @param[out] int * len      Return data length
     * @return const char *       value
     * @see tsdb_v3_t::query()
     */
    const char * ( * get_str_aligned )( tsdb_v3_reader_t * res, uint32_t field_index, int * len );

    /**
     * @brief Gets the specified field name.
     *        For the convenience of display, the returned data fills the remaining space with spaces.
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] uint32_t field_index
     * @param[out] int * len      Return field name length
     * @return const char * field_name
     * @see tsdb_v3_t::query()
     */
    const char * ( * get_field_aligned )( tsdb_v3_reader_t * res, uint32_t field_index, int * len );

    /**
     * @brief clear all the data, or (and) delete all field settings.
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] BOOL clear_data
     * @param[in] BOOL clear_field
     * @see tsdb_v3_t::query()
     */
    void ( * clear )( tsdb_v3_reader_t * res, BOOL clear_data, BOOL clear_field );

    /**
     * @brief Checks whether the value of the specified field in the current row is NULL
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] uint32_t field_index
     * @return BOOL  return TRUE if the value is NULL
     * @see tsdb_v3_t::query()
     */
    BOOL         ( * is_null       )( tsdb_v3_reader_t * res, uint32_t field_index );
    BOOL         ( * is_null_s     )( tsdb_v3_reader_t * res, const char * field );

    /**
     * @brief get the bool field value in the current row, return NULL if NULL
     * @param[in] tsdb_v3_reader_t *  res
     * @param[in] uint32_t field_index
     * @return byte_t *  return point of byte_t, 1 indicate TRUE, and 0 indicate FALSE
     * @see tsdb_v3_t::query()
     */
    byte_t *     ( * get_bool       )( tsdb_v3_reader_t * res, uint32_t field_index );
    int *        ( * get_int        )( tsdb_v3_reader_t * res, uint32_t field_index );
    int64_t *    ( * get_int64      )( tsdb_v3_reader_t * res, uint32_t field_index );
    int64_t *    ( * get_datetime_ms)( tsdb_v3_reader_t * res, uint32_t field_index );
    float *      ( * get_float      )( tsdb_v3_reader_t * res, uint32_t field_index );
    double *     ( * get_double     )( tsdb_v3_reader_t * res, uint32_t field_index );
    const char * ( * get_string     )( tsdb_v3_reader_t * res, uint32_t field_index, uint32_t * length );
    const void * ( * get_pointer    )( tsdb_v3_reader_t * res, uint32_t field_index );

    byte_t *     ( * get_bool_s         )( tsdb_v3_reader_t * res, const char * field );
    int *        ( * get_int_s          )( tsdb_v3_reader_t * res, const char * field );
    int64_t *    ( * get_int64_s        )( tsdb_v3_reader_t * res, const char * field );
    int64_t *    ( * get_datetime_ms_s  )( tsdb_v3_reader_t * res, const char * field );
    float *      ( * get_float_s        )( tsdb_v3_reader_t * res, const char * field );
    double *     ( * get_double_s       )( tsdb_v3_reader_t * res, const char * field );
    const char * ( * get_string_s       )( tsdb_v3_reader_t * res, const char * field, uint32_t * length );
    const void * ( * get_pointer_s      )( tsdb_v3_reader_t * res, const char * field );

    int         ( * row_add         )( tsdb_v3_reader_t * res );
    int         ( * set_null        )( tsdb_v3_reader_t * res, uint32_t field_index );
    int         ( * set_bool        )( tsdb_v3_reader_t * res, uint32_t field_index, BOOL v );
    int         ( * set_int         )( tsdb_v3_reader_t * res, uint32_t field_index, int v );
    int         ( * set_int64       )( tsdb_v3_reader_t * res, uint32_t field_index, int64_t v );
    int         ( * set_datetime_ms )( tsdb_v3_reader_t * res, uint32_t field_index, int64_t v );
    int         ( * set_float       )( tsdb_v3_reader_t * res, uint32_t field_index, float v );
    int         ( * set_double      )( tsdb_v3_reader_t * res, uint32_t field_index, double v );
    int         ( * set_string      )( tsdb_v3_reader_t * res, uint32_t field_index, const char * str, uint32_t len );
    int         ( * set_pointer     )( tsdb_v3_reader_t * res, uint32_t field_index, const void * v );
    int         ( * set_bool_ref    )( tsdb_v3_reader_t * res, uint32_t field_index, byte_t * v );
    int         ( * set_int_ref     )( tsdb_v3_reader_t * res, uint32_t field_index, int * v );
    int         ( * set_int64_ref   )( tsdb_v3_reader_t * res, uint32_t field_index, int64_t * v );
    int         ( * set_datetime_ms_ref )( tsdb_v3_reader_t * res, uint32_t field_index, int64_t * v );
    int         ( * set_float_ref   )( tsdb_v3_reader_t * res, uint32_t field_index, float * v );
    int         ( * set_double_ref  )( tsdb_v3_reader_t * res, uint32_t field_index, double * v );
    int         ( * set_string_ref  )( tsdb_v3_reader_t * res, uint32_t field_index, const char * str );
    int         ( * row_add_commit  )( tsdb_v3_reader_t * res );

    BOOL        ( * editing_mode_get )( tsdb_v3_reader_t * res );
    void        ( * editing_mode_set )( tsdb_v3_reader_t * res, BOOL v );

    uint32_t    ( * row_bytes )( tsdb_v3_reader_t * res );
    uint32_t    ( * row_waste_bytes )( tsdb_v3_reader_t * res );
    uint32_t    ( * row_block_bytes_get )( tsdb_v3_reader_t * res );
    void        ( * row_block_bytes_set )( tsdb_v3_reader_t * res, uint32_t v );

    //<private>
    /**
     * @brief is it has time field  
     * @return BOOL  is it has time field
     */
    //</private>
    BOOL        ( * has_timestamp_field )( tsdb_v3_reader_t * res );

    /**
     * @brief select current database
     * @param[in] tsdb_v3_reader_t * self
     * @param[in] const char * db       database name
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * select_db )(
        tsdb_v3_reader_t *  self,
        const char *        db
    );

    /**
     * @brief Set the values of all fields in the row being added in 'res'
     *        to the corresponding field values in the current row in 'from'.
     *        The field settings of both sides must be equal.
     * @param[in] tsdb_v3_reader_t * res - current reader. row_add* function must already called.
     * @param[in] tsdb_v3_reader_t * from- another reader that is data source. cursor_next must already called.
     * @return int.  error number, 0 indicate NO error.
     * @par Sample
     * @code
        int r;

        ///////////////////////////////////////////////////////////////
        // prepare src local table

        tsdb_v3_t * tsdb = tsdb_v3_tls( TSDB_V3_VERSION );
        assert( tsdb );

        // create a table object, type is 'local' table
        tsdb_v3_reader_t * src = tsdb->table_new( "local" );
        assert( src );

        // add a field called a, type is int
        r = src->field_add( src, "a", TSDB_DATATYPE_INT, 0, TRUE, NULL );
        assert( 0 == r );

        // add a row
        r = src->row_add( src );
        assert( 0 == r );
        // first field's type is int.
        r = src->set_int( src, 0, 12 );
        assert( 0 == r );
        // commit to add a row
        r = src->row_add_commit( src );
        assert( 0 == r );

        // add a row
        r = src->row_add( src );
        assert( 0 == r );
        // first field's type is int.
        r = src->set_int( src, 0, 13 );
        assert( 0 == r );
        // commit to add a row
        r = src->row_add_commit( src );
        assert( 0 == r );

        uint64_t src_row_count = src->row_count( src );
        assert( 2 == src_row_count );
        printf( "src's record_count = %lld\n", (long long)src_row_count );

        // prepare src local table
        ///////////////////////////////////////////////////////////////
        // prepare dst local table

        tsdb_v3_reader_t * dst = tsdb->table_new( "local" );
        assert( dst );

        // add a field called a, type is int
        r = dst->field_add( dst, "a", TSDB_DATATYPE_INT, 0, TRUE, NULL );
        assert( 0 == r );

        uint64_t dst_row_count = dst->row_count( dst );
        assert( 0 == dst_row_count );
        printf( "dst's record_count = %lld\n", (long long)dst_row_count );

        // prepare dst local table
        ///////////////////////////////////////////////////////////////
        // copy from src to dst

        r = src->cursor_reset( src );
        assert( 0 == r );
        while ( true ) {
            r = src->cursor_next( src );
            if ( 0 != r ) {
                break;
            }

            // add a row
            r = dst->row_add( dst );
            assert( 0 == r );

            // copy all fields value from 'src' to dest's current adding row
            r = dst->set_row_from( dst, src );
            assert( 0 == r );

            // commit to add a row
            r = dst->row_add_commit( dst );
            assert( 0 == r );
        }
        dst_row_count = dst->row_count( dst );
        assert( src_row_count == dst_row_count );
        printf( "dst's record_count = %lld\n", (long long)dst_row_count );

        // copy from src to dst
        ///////////////////////////////////////////////////////////////
        // print dst

        // print field line
        for ( uint32_t i = 0; i < dst->field_count( dst ); ++ i ) {
            tsdb_v3_field_t * field = dst->field_get( dst, i );
            assert( field );
            if ( 0 != i ) {
                printf( "\t" );
            }
            printf( "%s", field->name );
        }
        printf( "\n\n" );

        r = dst->cursor_reset( dst );
        assert( 0 == r );

        // print data lines
        while ( true ) {
            r = dst->cursor_next( dst );
            if ( 0 != r ) {
                break;
            }

            for ( uint32_t i = 0; i < dst->field_count( dst ); ++ i ) {
                if ( 0 != i ) {
                    printf( "\t" );
                }
                uint32_t len;
                const char * s = dst->get_string( dst, 0, & len );
                if ( NULL == s ) {
                    printf( "NULL" );
                } else {
                    fwrite( s, 1, len, stdout );
                }
            }
            printf( "\n" );
        }

        // print dst
        ///////////////////////////////////////////////////////////////

        src->kill_me( src );
        dst->kill_me( dst );
     * @endcode
     */
    int         ( * set_row_from  )( tsdb_v3_reader_t * res, tsdb_v3_reader_t * from );

    //<private>
    /**
     * @brief 设置 field_count() 函数返回的字段数量，本类用于限制把前面 count 个字段的数据给别人展示。  
     * @param[in] uint32_t count 希望 field_count() 函数返回的字段数量。  
     * @return int 错误码。  
     */
    //</private>
    int         ( * set_field_count_limit )( tsdb_v3_reader_t * res, uint32_t count );

    const char * ( * get_table_name )( tsdb_v3_reader_t * res );
    void         ( * set_table_name )( tsdb_v3_reader_t * res, const char * table_name );
};

/**
 * @brief Client interface object  
 */
struct tsdb_v3_t
{
    ///////////////////////////////////////////////////////////////////
    // Inner

    /// version of current interface, must be TSDB_V3_VERSION
    uint64_t                            version;

    // build version.
    const char *                        build_version;

    // inner object, caller never use it
    void *                              _inner;

    // user defined point
    void *                              user_defined_ptr;

    // General function library
#if defined( _RTDB_TSDB_V3_NO_TOOLS_ ) && _RTDB_TSDB_V3_NO_TOOLS_
    void *                              tools;
#else
    tsdb_tools_t *                      tools;
#endif

    /**
     * @brief delete current object
     * @param[in] tsdb_v3_t * self
     * @return void
     */
    void ( * kill_me )(
        tsdb_v3_t *                 self
    );

    // Inner
    ///////////////////////////////////////////////////////////////////

    /**
     * @brief get directory path that you can store user data in it.
     * @param[out]    char * dir    directory path buffer.
     * @param[in/out] int * dir_len input:  bytes in the dir buffer( including '\0')
     *                              output: bytes to be written ( not including '\0' )
     * @param[in] bool add_path_sep is add path separtor char to dir result.
     * @return BOOL is it OK
     * @par Sample
     * @code
        tsdb_v3_t * tsdb = tsdb_v3_tls_s( TSDB_V3_VERSION );
        char buf[ 256 ] = "";
        int  buf_len = (int)sizeof(buf);
        if ( ! tsdb->get_basedir( buf, & buf_len, false ) ) {
            printf( "get_basedir failed\n" );
            return -1;
        }
        printf( "%s\n", buf );
     * @endcode
     */
    BOOL ( * get_basedir )( char * dir, int * dir_len, BOOL add_path_sep );

    /**
     * @brief get directory path that you can store user data in it.
     * @param[out]    char * dir    directory path buffer.
     * @param[in/out] int * dir_len input:  bytes in the dir buffer( including '\0')
     *                              output: bytes to be written ( not including '\0' )
     * @param[in] bool add_path_sep is add path separtor char to dir result.
     * @return BOOL is it OK
     * @par Sample
     * @code
        tsdb_v3_t * tsdb = tsdb_v3_tls_s( TSDB_V3_VERSION );
        char buf[ 256 ] = "";
        int  buf_len = (int)sizeof(buf);
        if ( ! tsdb->get_logdir( buf, & buf_len, false ) ) {
            printf( "get_logdir failed\n" );
            return -1;
        }
        printf( "%s\n", buf );
     * @endcode
     */
    BOOL ( * get_logdir )( char * dir, int * dir_len, BOOL add_path_sep );

    /**
     * @brief connect to a server
     * @param[in] const char * conn_str  connection string like:
     *                                   server=127.0.0.1:9000;user=test;passwd=test
     * @return int.  error number, 0 indicate NO error.
     * @warning      The impact of the function is global in the process.
     *               As long as a connect successfully once, other connection
     *               needn't to connect again.
     * @par Sample
     * @code

        // Ignore error check for easy reading.

        // @see tsdb_v3_tls_t
        // @see tsdb_v3_tls_s()
        // @see tsdb_v3_new_t
        // @see tsdb_v3_new_s()
        tsdb_v3_t * tsdb = tsdb_v3_tls_s( TSDB_V3_VERSION );

        int r = 0;

        do {

            // login if not
            if ( ! tsdb->is_logined() ) {
                r = tsdb->connect( "server=127.0.0.1:9000;user=test;passwd=test" );
                if ( 0 != r ) {
                    printf( "[r=%d]connect failed\n", r );
                    break;
                }
            }

            // do somthing with connected tsdb ...

        } while ( 0 );

     * @endcode
     */
    int ( * connect )(
        const char *                conn_str
    );

    /**
     * @brief disconnect from server, this is a global operation
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * disconnect )();

    /**
     * @brief Is the current login successful, this is a global operation
     * @return BOOL is_logined
     */
    BOOL ( * is_logined )();

    ///////////////////////////////////////////////////////////////////

    /**
     * @brief connect to a server
     * @param[in] const char * conn_str  connection string like:
     *                                   server=127.0.0.1:9000;user=test;passwd=test
     * @return int.  error number, 0 indicate NO error.
     * @warning      Valid only for current connection.
     * @par Sample
     * @code

        // Ignore error check for easy reading.

        // @see tsdb_v3_tls_t
        // @see tsdb_v3_tls_s()
        // @see tsdb_v3_new_t
        // @see tsdb_v3_new_s()
        tsdb_v3_t * tsdb = tsdb_v3_tls_s( TSDB_V3_VERSION );

        int r = 0;

        do {

            // login if not
            if ( ! tsdb->is_logined_private( tsdb ) ) {
                r = tsdb->connect_private( tsdb, "server=127.0.0.1:9000;user=test;passwd=test" );
                if ( 0 != r ) {
                    printf( "[r=%d]connect_private failed\n", r );
                    break;
                }
            }

            // do somthing with connected tsdb ...
            tsdb->disconnect_private( tsdb );

        } while ( 0 );

     * @endcode
     */
    int ( * connect_private )(
        tsdb_v3_t *                 self,
        const char *                conn_str
    );

    /**
     * @brief disconnect from server.
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * disconnect_private )(
        tsdb_v3_t *                 self
    );

    /**
     * @brief Is the current login successful, this is a global operation
     * @return BOOL is_logined
     */
    BOOL ( * is_logined_private )(
        tsdb_v3_t *                 self
    );

    ///////////////////////////////////////////////////////////////////

    const char * ( * charset_get )();
    int ( * charset_set )( const char * charset );

    /**
     * @brief If the login is successful, take the user name
     * @param[in] tsdb_v3_t * self
     * @return const char * user_name or ""
     */
    const char * ( * user_name )(
        tsdb_v3_t *                 self
    );

    /**
     * @brief If the login is successful, take the server address
     * @param[in] tsdb_v3_t * self
     * @return const char * server_address or ""
     */
    const char * ( * server_addr_str )(
        tsdb_v3_t *                 self
    );

    ///////////////////////////////////////////////////////////////////

    /**
     * @brief If you want to visit PostgreSql DB's content, you need to init postgresql library first.
     * @param[in] const char * libpq_path    If you want to initialize Postgresql, pass libpq.dll path
     *                                       If you want to unload Postgresql, pass NULL or ""
     * @param[out] int * version             return PQlibVersion return value
     * @return error code
     */
    int ( * pg_init )( const char * libpq_path, int * version );

    ///////////////////////////////////////////////////////////////////
    //
    // mysql likely query
    //

    /**
     * @brief Create a local table container object
     *        you should call tsdb_v3_reader_t::kill_me to delete it.
     * @param[in] const char * type
     *                             "local": local table
     *                             "odbc" : ODBC DB connector
     *                             "pg"   : postgresql compactibility DB connector
     *                             "mysql": MYSQL compactibility DB connector
     * @return tsdb_v3_reader_t *
     * @par Sample
     * @code

        RTDB * rtdb = tsdb_v3_tls_s( TSDB_V3_VERSION );

        RTDB_RES * res = rtdb->table_new( "local" );

        int r;
        r = res->field_add( res, "field_1", TSDB_DATATYPE_STRING, 128 ); assert( 0 == r );
        r = res->field_add( res, "field_2", TSDB_DATATYPE_INT, 0 );      assert( 0 == r );

        r = res->row_add( res );                              assert( 0 == r );
        const char s[] = "abcdefg";
        r = res->set_string( res, s, (uint32_t)sizeof(s)-1 ); assert( 0 == r );
        r = res->set_int( res, 432789 );                      assert( 0 == r );
        r = res->row_add_commit( res );                       assert( 0 == r );

        while ( 0 == ( r = res->cursor_next( res ) ) ) {

            uint32_t f1_len;
            const char * f1 = res->get_string( res, 0, & s2_len );

            int * f2 = res->get_int( res, 1 );

            printf( "%s, %d\n", f1, f2 );
        }

        res->kill_me( res );

     * @endcode
     */
    tsdb_v3_reader_t * ( * table_new )( const char * type );

    /**
     * @brief load CSV file content into a tsdb_v3_reader_t object
     * @param[in] const char *       path
     * @param[in] tsdb_v3_reader_t * reader     reader
     * @return int.  error number, 0 indicate NO error.
     * @par Sample
     * @code

        // Ignore error check for easy reading.

        // @see tsdb_v3_tls_t
        // @see tsdb_v3_tls_s()
        // @see tsdb_v3_new_t
        // @see tsdb_v3_new_s()
        RTDB * rtdb = tsdb_v3_tls_s( TSDB_V3_VERSION );
        tsdb_v3_reader_t * reader = rtdb->local_table_new();
        int r = rtdb->load_csv_file( "./path", reader );
        reader->kill_me( reader );

     * @endcode
     */
    int ( * load_csv_file )(
        const char *        path,
        tsdb_v3_reader_t *  reader
    );

    /**
     * @brief get current DB name
     * @param[in] tsdb_v3_reader_t * reader     reader
     * @return const char * DB name, empty sting if no current DB
     */
    const char * ( * db_current )(
        tsdb_v3_t *                 self
    );

    /**
     * @brief execute a sql statement
     * @param[in] tsdb_v3_t *  self     
     * @param[in] const char * sql      sql statement
     * @param[in] int          sql_len  sql statment length, not inluding \0
     * @param[in] const char * database database name, optional
     * @return int.  error number, 0 indicate NO error.
     * @par Sample
     * @code

        // Ignore error check for easy reading.

        // @see tsdb_v3_tls_t
        // @see tsdb_v3_tls_s()
        // @see tsdb_v3_new_t
        // @see tsdb_v3_new_s()
        RTDB * rtdb = tsdb_v3_tls_s( TSDB_V3_VERSION );

        int r = 0;

        do {
            const char sql[] = "select * from table";
            r = rtdb->query( tsdb, sql, (int)strlen(sql), NULL );
            if ( 0 != r ) {
                printf( "[r=%d]query failed\n", r );
                break;
            }

            RTDB_RES *      res             = rtdb->store_result( rtdb );
            uint64_t        row_count       = rtdb->row_count( res );
            uint32_t        column_count    = rtdb->field_count( res );
            RTDB_FIELD *    field           = rtdb->fetch_field( res, & column_count );

            for ( uint32_t i = 0; i < column_count; ++ i ) {
                printf( "Field %d is %s\n", i, field[ i ].name );
            }
            printf( "\n" );

            while ( true ) {
                r = rtdb->cursor_next( res );
                if ( 0 != r ) {
                    break;
                }
                for (t = 0; t < column_count; t++) {
                    printf(" %s\t", row[t]);
                }
                printf("\n");
            }

        } while ( 0 );

     * @endcode
     */
    int ( * query )(
        tsdb_v3_t *                 self,
        const char *                sql,
        int                         sql_len,
        const char *                charset,
        const char *                database
    );

    /**
     * @brief execute a sql statement and return a reader
     * @param[in] tsdb_v3_t *  self     
     * @param[in] const char * sql      sql statement
     * @param[in] int          sql_len  sql statment length, not inluding \0
     * @param[in] const char * database database name, optional
     * @param[in] BOOL fetch_first_line If this value is TRUE, auto call cursor_next
     * @param[in] int r                 error code, if no recordset, r is ENODATA and return NULL
     * @return tsdb_v3_reader_t * reader caller needn't free it.
     */
    tsdb_v3_reader_t * ( * query_reader )(
        tsdb_v3_t *                 self,
        const char *                sql,
        int                         sql_len,
        const char *                charset,
        const char *                database,
        BOOL                        fetch_first_line,
        int *                       r
    );

    /**
     * @brief Gets the context of the search results. 
     *        If the current query statement does not return a result data,
     *        this function returns NULL
     * @param[in] tsdb_v3_t *  self     
     * @return RTDB_RES * search result data context.
     * @see query()
     */
    tsdb_v3_reader_t * ( * store_result )( tsdb_v3_t * self );

    //
    // mysql likely query
    //
    ///////////////////////////////////////////////////////////////////

    int ( * set_timeout )(
        int     connect_timeout_ms,
        int     send_timeout_ms,
        int     recv_timeout_ms
    );

    int ( * get_timeout )(
        int *   connect_timeout_ms,
        int *   send_timeout_ms,
        int *   recv_timeout_ms
    );

    /**
     * @brief insert binary data
     * @param[in] tsdb_v3_t * self
     * @param[in] const char * db       database name
     * @param[in] const char * type     data type string
     * @param[in] const char * data     data
     * @param[in] int data_len          data bytes
     * @param[out] uint32_t * affected  written lines
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * insert )(
        tsdb_v3_t *     self,
        const char *    db,
        const char *    type,
        const char *    data,
        int             data_len,
        uint32_t *      affected
    );

    /**
     * @brief select current database
     * @param[in] tsdb_v3_t * self
     * @param[in] const char * db       database name
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * select_db )(
        tsdb_v3_t *     self,
        const char *    db
    );

    /**
     * @brief get data type info
     * @param[in] int datatype  TSDB_DATATYPE_*
     * @return const tsdb_datatype_info_t * point to datatype info
     */
    const tsdb_datatype_info_t * ( * datatype_info )( int datatype );
    const tsdb_datatype_info_t * ( * datatype_find )( const char * name, int name_len );

    /**
     * @brief For functional unit testing,
     * @param[in] tsdb_v3_t * self
     * @param[in] int         argc
     * @param[in] char **     argv
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * test )( tsdb_v3_t * self, int argc, char ** argv );

    /**
     * @brief For performance testing,
     *        construct a binary packet with a specified bytes
     *        and send it to the server.
     *        The server also returns a packet with same length.
     *        The client verifies it and returns it after success.
     * @param[in] tsdb_v3_t * self
     * @param[in] int         req_bytes request packet bytes
     * @param[in] int         rsp_bytes response packet bytes
     * @return int.  error number, 0 indicate NO error.
     */
    int ( * test_call )(
        tsdb_v3_t *                 self,
        int                         req_bytes,
        int                         rsp_bytes
    );

    /**
     * @brief print result as string
     * @param[in] tsdb_v3_t * self
     * @param[in] const char * parameters. optional extent parameters, support NULL.
     * @param[out] const char * * str return point to string, caller should not delete it.
     *             This value is valid before next time to call print function.
     *             The data will be write to stdout and fflush( stdout ), if this parameter is NULL.
     * @param[out] int * str_len      str length by char. not including '\0'
     * @return int.  error number, 0 indicate NO error.
     * @par Sample
     * @code
        // Ignore error check for easy reading.
        // @see tsdb_v3_tls_t
        // @see tsdb_v3_tls_s()
        // @see tsdb_v3_new_t
        // @see tsdb_v3_new_s()
        RTDB * rtdb = tsdb_v3_tls_s( TSDB_V3_VERSION );

        int r = 0;

        const char sql[] = "select * from table";
        r = rtdb->query( tsdb, sql, (int)strlen(sql), NULL );
        if ( 0 != r ) {
            printf( "[r=%d]query failed\n", r );
            goto err;
        }

        // direct write data into stdout
        rtdb->print( rtdb, NULL, NULL, NULL );

        // OR, write data into a string, then we write it to stdout by manual.
        const char * str;
        int          str_len;
        rtdb->print( rtdb, NULL, & str, & str_len );
        fwrite( str, 1, (size_t)str_len, stdout );
        fflush( stdout );
     * @endcode
     */
    int ( * print )(
        tsdb_v3_t *                 self,
        const char *                parameters,
        const char * *              str,
        int *                       str_len
    );

    /**
     * @brief call this if user press Ctrl+C or close the client window
     */
    void ( * user_break )();

};

///////////////////////////////////////////////////////////////////////
//

#define TSDB_V3_NEW_FUNC_NAME   "tsdb_v3_new"
#define TSDB_V3_TLS_FUNC_NAME   "tsdb_v3_tls"

typedef tsdb_v3_t * ( * tsdb_v3_new_t )( uint64_t version );
typedef tsdb_v3_t * ( * tsdb_v3_tls_t )( uint64_t version );

#if defined( _TSDB_SVR ) || defined( TSDB_CLI ) || defined( CLIENT_EXPORTS )

    tsdb_v3_t * tsdb_v3_new_s( uint64_t version );
    tsdb_v3_t * tsdb_v3_tls_s( uint64_t version );

#else

    #define TSDB_ERROR( p, str, ...)      (p)->tools->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_ERR,     true, str, ##__VA_ARGS__ )
    #define TSDB_INFO( p, str, ...)       (p)->tools->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_INF,    false, str, ##__VA_ARGS__ )

    #if defined( _WIN32 ) || defined( _WIN64 )
        static inline dll_t _tsdb_v3_inner_dll_open( const char * path ) { return LoadLibraryA( path ); }
        static inline void  _tsdb_v3_inner_dll_close( dll_t h ) { if ( h ) { FreeLibrary( h ); } }
        static inline void* _tsdb_v3_inner_dll_symbol( dll_t h, const char * func_name ) { if ( h && func_name && * func_name ) { return GetProcAddress( h, func_name ); } return NULL; }
    #else
        static inline bool  _tsdb_v3_inner_add_dir_to_env( const char * so ) { const char * old_env; size_t old_env_len; char dir[ 260 ]; size_t dir_len; const char * p; char buf[ 1024 ]; int r; p = strrchr( so, S_PATH_SEP_C ); if ( unlikely( NULL == p ) ) { return false; } if ( unlikely( (size_t)(p - so) >= sizeof( dir ) ) ) { return false; } dir_len = p - so; memcpy( dir, so, dir_len ); dir[ dir_len ] = '\0'; old_env = getenv( "LD_LIBRARY_PATH" ); if ( NULL == old_env ) { old_env = ""; } p = strstr( old_env, dir ); if ( p ) { bool match = true; do { if ( p > old_env && ':' != * ( p - 1 ) ) { match = false; break; } if ( '\0' != p[ dir_len ] && ':' != p[ dir_len ] ) { match = false; break; } } while ( 0 ); if ( match ) { return true; } } old_env_len = strlen( old_env ); if ( (size_t)(sizeof("LD_LIBRARY_PATH=")-1 + old_env_len + sizeof(":")-1 + dir_len) < sizeof( buf ) ) { strcpy( buf, "LD_LIBRARY_PATH=" ); strcat( buf, old_env ); strcat( buf, ":" ); strcat( buf, dir ); r = putenv( buf ); if ( unlikely( 0 != r ) ) { return false; } } else { char * p; p = (char *)malloc( sizeof("LD_LIBRARY_PATH=")-1 + old_env_len + sizeof(":")-1 + dir_len + 1 ); if ( unlikely( NULL == p ) ) { return false; } strcpy( buf, "LD_LIBRARY_PATH=" ); if ( old_env_len > 0 ) { strcat( buf, old_env ); strcat( buf, ":" ); } strcat( buf, dir ); r = putenv( buf ); if ( unlikely( 0 != r ) ) { free( p ); return false; } free( p ); } if ( unlikely( 0 != r ) ) { return false; } return true; }
        static inline dll_t _tsdb_v3_inner_dll_open( const char * path ) { if ( _tsdb_v3_inner_add_dir_to_env( path ) ) { return dlopen( path, RTLD_NOW ); } return NULL; }
        static inline void  _tsdb_v3_inner_dll_close( dll_t h ) { if ( h ) { dlclose( h ); } }
        static inline void* _tsdb_v3_inner_dll_symbol( dll_t h, const char * func_name ) { if ( h && func_name && * func_name ) { return dlsym( h, func_name ); } return NULL; }
    #endif

    struct tsdb_v3_init_t
    {
        dll_t           dll;
        tsdb_v3_tls_t   func_tls;
        tsdb_v3_new_t   func_new;
    };
    typedef struct tsdb_v3_init_t tsdb_v3_init_t;

    /**
     * @brief initialize the TSDB library dll/so
     * @param[in] const char * path   tsdb.dll | libtsdb.so file path, if NULL then use default file name.
     * @return tsdb_v3_init_t * If successed, return address of init struct, NULL if failed.
     * @par Sample
     * @code

            // Ignore error check for easy reading.

            // load library and find function
            if ( NULL == tsdb_v3_init( NULL ) ) {
                printf( "tsdb_v3_init failed!!\n" );
                return -1;
            }

            // get interface object for current thread
            tsdb_v3_t * tsdb = tsdb_v3_tls( TSDB_V3_VERSION );
            if ( NULL == tsdb ) {
                printf( "tsdb_v3_tls failed!!\n" );
                return -1;
            }

            // use tsdb in current thread...
     * @endcode
     */
    static inline tsdb_v3_init_t * tsdb_v3_init( const char * path )
    {
        static tsdb_v3_init_t g_init;
        if ( g_init.dll && g_init.func_tls && g_init.func_new ) {
            return & g_init;
        }
        if ( unlikely( NULL == path || '\0' == * path ) ) {
#if defined( _WIN32 ) || defined( _WIN64 )
            path = "tsdb.dll";
#else
            do {
                struct stat st;
                memset( & st, 0, sizeof(st) );
                if ( stat( "./libtsdb.so", & st ) != -1 && 0 == (st.st_mode & S_IFDIR) ) {
                    path = "./libtsdb.so";
                    break;
                }
                memset( & st, 0, sizeof(st) );
                if ( stat( "/usr/lib/libtsdb.so", & st ) != -1 && 0 == (st.st_mode & S_IFDIR) ) {
                    path = "/usr/lib/libtsdb.so";
                    break;
                }
                memset( & st, 0, sizeof(st) );
                if ( stat( "./librtdb.so", & st ) != -1 && 0 == (st.st_mode & S_IFDIR) ) {
                    path = "./librtdb.so";
                    break;
                }
                memset( & st, 0, sizeof(st) );
                if ( stat( "/usr/lib/librtdb.so", & st ) != -1 && 0 == (st.st_mode & S_IFDIR) ) {
                    path = "/usr/lib/librtdb.so";
                    break;
                }
                path = "libtsdb.so";
            } while ( 0 );
#endif
        }
        {
            g_init.dll = _tsdb_v3_inner_dll_open( path );
            if ( g_init.dll ) {
                g_init.func_new = (tsdb_v3_new_t)_tsdb_v3_inner_dll_symbol( g_init.dll, TSDB_V3_NEW_FUNC_NAME );
                g_init.func_tls = (tsdb_v3_tls_t)_tsdb_v3_inner_dll_symbol( g_init.dll, TSDB_V3_TLS_FUNC_NAME );
                if ( g_init.func_tls && g_init.func_new ) {
                    return & g_init;
                }
            }
        }
        return NULL;
    }

    /**
     * @brief get a tsdb_v3_t object from TLS, caller needn't delete it
     * @param[in] int version  must be TSDB_B3_VERSION  
     * @return tsdb_v3_t * pointer of object, caller needn't delete it
     * @par Sample
     * @code

            // Ignore error check for easy reading.

            // load library and find function
            if ( NULL == tsdb_v3_init( NULL ) ) {
                printf( "tsdb_v3_init failed!!\n" );
                return -1;
            }

            // get interface object for current thread
            tsdb_v3_t * tsdb = tsdb_v3_tls( TSDB_V3_VERSION );
            if ( NULL == tsdb ) {
                printf( "tsdb_v3_tls failed!!\n" );
                return -1;
            }

            // use tsdb in current thread...
     * @endcode
     */
    static inline tsdb_v3_t * tsdb_v3_tls( uint64_t version )
    {
        tsdb_v3_init_t * p = tsdb_v3_init( NULL );
        if ( p && p->func_tls ) {
            return p->func_tls( version );
        }
        return NULL;
    }

    /**
     * @brief create a tsdb_v3_t object, caller must use kill_me function to delete the object
     * @param[in] int version  must be TSDB_B3_VERSION  
     * @return tsdb_v3_t * pointer of object, delete by kill_me function
     * @code

        // Ignore error check for easy reading.

        // load library and find function
        if ( NULL == tsdb_v3_init( NULL ) ) {
            printf( "tsdb_v3_init failed!!\n" );
            return -1;
        }

        // create interface object for current process
        tsdb_v3_t * tsdb = tsdb_v3_new( TSDB_V3_VERSION );
        if ( NULL == tsdb ) {
            printf( "tsdb_v3_new failed!!\n" );
            return -1;
        }

        // use tsdb ...

        tsdb->kill_me( tsdb );
     * @endcode
     */
    static inline tsdb_v3_t * tsdb_v3_new( uint64_t version )
    {
        tsdb_v3_init_t * p = tsdb_v3_init( NULL );
        if ( p && p->func_new ) {
            return p->func_new( version );
        }
        return NULL;
    }

#endif // #if ! defined( _TSDB_SVR ) && ! defined( TSDB_CLI )

#ifdef __cplusplus
}
#endif

#endif

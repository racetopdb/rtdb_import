// YOU DON'T NEED TO CHANGE THIS FILE !!!!!!!!!

/**
 * @file include/gr_stdinc.h
 * @author zouyueming(da_ming at hotmail.com)
 * @date 2013/09/24
 * @version $Revision$
 * @brief standard include header
 *
 * Revision History
 *
 * @if  ID       Author       Date          Major Change       @endif
 *  ---------+------------+------------+------------------------------+\n
 *       1     zouyueming   2013-09-24    Created.
 *       2     zouyueming   2014-10-06    add BOOL, TRUE, FALSE for non windows.
 *       3     zouyueming   2014-10-07    add android support
 *       4     zouyueming   2014-11-09    Mac OS X yosemite add CHAR_MAX in limits.h
 *       5     zouyueming   2015-02-19    add _exit function on windows.
 *       6     zouyueming   2015-03-17    fixed build error on android
 *       7     zouyueming   2016-06-19    fixed build error after MFC headers
 *       8     zouyueming   2021-08-04    add safe_snprintf MACRO function,
 *                                        avoid snprintf function difference return value
 *                                        on difference OS.
 *       9     zouyueming   2021-09-22    uint32_t dpr_log( uint32_t pow_value, uint32_t base )
 *      10     zouyueming   2021-10-03    fixed mktime's bug when.
 *      11     zouyueming   2021-10-10    add nullptr support on before c++11
 *      12     zouyumeing   2022-02-11    avoid build warning on OFFSET_RECORD macro
 **/
/*
 * Copyright (C) 2013-now da_ming at hotmail.com
 * All rights reserved.
 *
 */
#ifndef _GROCKET_INCLUDE_GRSTDINC_H_
#define _GROCKET_INCLUDE_GRSTDINC_H_

#if ENCRYPT_FUNCTION_NAME
#include "dprsvr_safe.h"
#endif // #if ENCRYPT_FUNCTION_NAME

#define MAX_YEAR    9999

#if defined( _WIN32 )
    #ifndef WIN32
        #define WIN32
    #endif
#elif defined( _WIN64 )
    #ifndef WIN64
        #define WIN64
    #endif
#endif

#if ( defined( _WIN32 ) || defined( _WIN64 ) ) && defined( _DEBUG )
    #define CRTDBG_MAP_ALLOC
    //#define _CRTDBG_MAP_ALLOC_NEW
    #include <stdlib.h>
    #include <crtdbg.h>
    //#define new new( _CLIENT_BLOCK, __FILE__, __LINE__)
#endif

// hash_map warning
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// UINT16_MAX
#include <assert.h>
#include <errno.h>

//<private>
/*
#if defined( _WIN32 ) || defined( _WIN64 )
#else
    #include <sys/wait.h>
    #include <dirent.h>

    #if defined( __APPLE__ )
        // What fucken means? Fuck Apple!
        #ifdef __header_always_inline
            #undef __header_always_inline
        #endif
        #define __header_always_inline  static inline
    #endif
    #include <signal.h>     // for kill
#endif
*/
//</private>

#if defined( _WIN32 ) || defined( _WIN64 )

    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif

//<private>
    // Consider using _snprintf_s instead. To disable deprecation, use _CRT_SECURE_NO_DEPRECATE.
//</private>
    #pragma warning(disable:4996)

//<private>
    // The file contains a character that cannot be represented in the current code page (936). Save the file in Unicode format to prevent data loss
//</private>
    #pragma warning(disable:4819)

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
    #include <wsipx.h>
    #include <process.h>    // _beginthreadex
    #include <io.h>
    #include <Tlhelp32.h>

    #pragma comment( lib, "Iphlpapi.lib" )
    #pragma comment( lib, "ws2_32.lib" )

    #define __attribute__(X)

//<private>
    /* Standard file descriptors.  */
//</private>
    #define STDIN_FILENO    0       /* Standard input.  */
    #define STDOUT_FILENO   1       /* Standard output.  */
    #define STDERR_FILENO   2       /* Standard error output.  */

    #define _exit( r )      TerminateProcess( GetCurrentProcess(), (r) )

    //<private>
    // disk full.
    //</private>
    #define EXFULL          54

#else
    //<non_windows>

//<private>
    // Mac must define __USE_GNU before pthread.h
//</private>
    #ifndef __USE_GNU
        #define __USE_GNU
    #endif
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif

    #if defined( __linux )
        #include <linux/unistd.h>   // for __NR_gettid
        #if ! defined( __ANDROID__ )
            #include <bits/sockaddr.h>  // for sa_family_t
        #endif
    #endif

    #include <sys/un.h>             // for sockaddr_un
    #include <dlfcn.h>
    #include <pthread.h>
    #include <ctype.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/time.h>   // gettimeofday
    #include <stdint.h>
    #include <stdarg.h>
    #include <sys/stat.h>
    #include <strings.h>
    #include <sys/wait.h>
    #include <dirent.h>
    //</non_windows>

#endif

//<non_windows>
#if defined( __APPLE__ )
    #include <sys/un.h> // for sockaddr_un
    #include <TargetConditionals.h>
    #ifndef CHAR_MAX
        #define CHAR_MAX    INT8_MAX
    #endif
#endif
//</non_windows>

#include <stddef.h>     // for size_t on no windows
#include <errno.h>      // for errno
#include <string.h>     // for memset
#include <memory.h>
#include <limits.h>     // for INT_MAX
#include <time.h>
#include <math.h>

//<non_windows>
// _LIB
#if ( defined(__APPLE__) && ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR ) )
    #define _LIB
#endif
//</non_windows>

//<private>
// don't use max & min macros, it's doesn't cross iOS & Mac OS X
// std::max
// std::min
//</private>
#if ! defined( __ANDROID__ )
#ifdef __cplusplus
//<private>
    // std::max disabled on windows, and max disabled on Mac OS X & iOS..... faint!
    // we only add std::max & std::min on windows, because we can not add max & min macros on iOS...
//</private>
    #include <algorithm>

    #if defined( _WIN32 ) || defined( _WIN64 )
//<private>
        // compatible after MFC headers
//</private>
        #if ! defined( __AFX_H__ )
//<private>
            // This is my beauty work!!!!!!!!!!!!! so std::max & std::min can used on windows
//</private>
            #undef max
            #undef min
        #endif
    #endif // #if defined( _WIN32 ) || defined( _WIN64 )
#endif // #ifdef __cplusplus
#endif

//<private>
///////////////////////////////////////////////////////////////////////
//
// nullptr
//
//</private>
/*#ifdef __cplusplus
    #if __cplusplus <= 199711L
        #ifndef nullptr
            #define nullptr     NULL
        #endif
    #endif
#endif*/

//<private>
///////////////////////////////////////////////////////////////////////
//
// _DEBUG       ( 1 or not define )
// DEBUG        ( 1 or not define )
// NDEBUG       ( 1 or not define )
//
//</private>

#if defined( _DEBUG )
    #if ! defined( DEBUG )
        #define DEBUG   1
    #endif
    #if defined( NDEBUG )
        #undef NDEBUG
    #endif
#elif defined( DEBUG )
    #if ! defined( _DEBUG )
        #define _DEBUG  1
    #endif
    #if defined( NDEBUG )
        #undef NDEBUG
    #endif
#else
    // release
    #if ! defined( NDEBUG )
        #define NDEBUG  1
    #endif
    #if defined( _DEBUG )
        #undef _DEBUG
    #endif
    #if defined( DEBUG )
        #undef DEBUG
    #endif
#endif

//<private>
//
// S_BIG_ENDIAN            1 or 0
// S_LITTLE_ENDIAN         1 or 0
//
//</private>
#if defined( _WIN64 ) || defined( _WIN32 ) || defined(__i386) || defined(_M_IX86) || defined (__x86_64)
    #define S_LITTLE_ENDIAN     1
    #define S_BIG_ENDIAN        0
    //<non_windows>
#elif defined(__sparc) || defined(__sparc__) || defined(__hppa) || defined(__ppc__) || defined(_ARCH_COM)
    #define S_BIG_ENDIAN        1
    #define S_LITTLE_ENDIAN     0
#elif defined(_WIN32_WCE)
    #define S_LITTLE_ENDIAN     1
    #define S_BIG_ENDIAN        0
#elif defined( __APPLE__ )
    #if ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
//<private>
        // IOS
//</private>
        #define S_LITTLE_ENDIAN     1
        #define S_BIG_ENDIAN        0
    #else
//<private>
        // Mac OS X
//</private>
        #define S_LITTLE_ENDIAN     1
        #define S_BIG_ENDIAN        0
    #endif
#elif defined( __ANDROID__ )
    #define S_LITTLE_ENDIAN     1
    #define S_BIG_ENDIAN        0
#elif defined( _OPENWRT )
    #define S_LITTLE_ENDIAN     1
    #define S_BIG_ENDIAN        0
    //</non_windows>
#else
    #error Unknown architecture
#endif

#ifndef __APPLE__
    #if S_LITTLE_ENDIAN
        #define htonll( val )   ( ( (uint64_t)htonl( (uint32_t)(val) ) ) << 32 ) + htonl( (uint32_t)( (val) >> 32 ) )
        #define ntohll( val )   ( ( (uint64_t)ntohl( (uint32_t)(val) ) ) << 32 ) + ntohl( (uint32_t)( (val) >> 32 ) )
    #else
        #define htonll( val )   (val)
        #define ntohll( val )   (val)
    #endif
#endif

//<private>
// inline
//</private>
#if ! defined( __cplusplus )
    #if defined(_MSC_VER)
        #define inline  __inline
    #elif defined(DIAB_COMPILER)
//<private>
        // only pragmas supported, don't bother
//</private>
        #define inline
    #else
        #define inline
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

#if ! defined( INFINITE_DEFINED )
    #define INFINITE_DEFINED    1
    #if defined( __APPLE__ )
        #define INFINITE    0xFFFFFFFF  // Infinite timeout
    #elif defined( __FreeBSD__ )
        #define INFINITE    0xFFFFFFFF  // Infinite timeout
    #elif ! defined( _WIN32 ) && ! defined( _WIN64 )
        #define INFINITE    0xFFFFFFFF  // Infinite timeout
    #endif
#endif

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

//<private>
/**
 * @function OFFSET_RECORD macro
 * @brief 
 * @param[in] void * address: input pointer
 * @param[in] type          : the input pointer is a data type( called type ) member.
 * @param[in] field         : the input pointer is a data type member, called field
 * @return type * : return the wrapper type.
 * @author zouyueming
 * @date 2013/09/24
 * @code

   typedef struct {
       int number;
   } test_t;

   test_t   test;
   int *    a   = & test.number;
   test_t * b   = OFFSET_RECORD( a, test_t, number );
   assert( b == & test );
 * @endcode
 */
//</private>
#define OFFSET_RECORD(address, type, field) ((type *)(  \
    (char*)(address) - offsetof(type, field)            \
))

//<private>
/**
 * @function ALIGN_UP macro
 * @brief align a number up to a special number.
 * @param[in] number     : input number
 * @param[in] align_size : align bytes
 * @return number : number already align by align_size.
 * @author zouyueming
 * @date 2013/09/24
 * @code

   assert( 4 == ALIGN_UP( 3, 4 ) );

 * @endcode
 */
//</private>
#define ALIGN_UP( number, align_size )   \
    ( ( 0 != ( (number) % (align_size) ) ) ? ( (number) - ( (number) % (align_size) ) + (align_size) ) : (number) )

#define ALIGN_DOWN( number, align_size )   \
    ( ( 0 != ( (number) % (align_size) ) ) ? ( (number) - ( (number) % (align_size) ) ) : (number) )

//<private>
// isdigit
//</private>
#undef  isdigit
#define isdigit( C )    ( (C) >= '0' && (C) <= '9' )

//<private>
// isspace
//</private>
static inline int isspace2( char c )
{
    switch ( c )
    {
    case 0x20:  // space
    case 0x09:  // \t
    case 0x0A:  // \r
    case 0x0D:  // \n
    case 0x00:  // \0
    case 0x0B:  // \v
    case 0x0C:  // \f
        return 1;
    default:
        return 0;
    }
}
#undef  isspace
#define isspace( C )    isspace2( (C) )

//<private>
    // bool
    // true
    // false
//</private>
#ifndef __cplusplus
//<private>
    // Mac OS & iOS defined bool, so we add #ifndef bool #define bool ... #endif
//</private>
    #ifndef bool
        #define	bool    unsigned char
    #endif
    #define	true        1
    #define	false       0
#endif


//<private>
// abs64
// abs
// BOOL
//</private>
#if defined( _WIN32 ) || defined( _WIN64 )

    #define abs64(x)        _abs64((x))

    #if ! defined( BOOL_DEFINED )
        #define BOOL_DEFINED    1
    #endif // #if ! defined( BOOL_DEFINED )

#else
    //<non_windows>

    #define abs64(x)        llabs((x))

    #if ! defined( BOOL_DEFINED )
        #define BOOL_DEFINED    1
        typedef int BOOL;
    #endif // #if ! defined( BOOL_DEFINED )
    #ifndef FALSE
        #define FALSE               0
    #endif
    #ifndef TRUE
        #define TRUE                1
    #endif

#endif
//<private>
//2022-05-22 fixed abs bug for long long parameter
//2022-07-26 remove #define abs(x) abs64((x)), because this not compatible with clickhouse
//</private>
//#define     abs(x)          abs64((x))



static inline int dpr_timezone()
{
    static int    g_dpr_datetime_timezone          = 0;
    static int    g_dpr_datetime_timezone_valid    = 0;
    if ( unlikely( ! g_dpr_datetime_timezone_valid ) ) {

#if defined( _WIN32 )
        time_t          t1;
        time_t          t2;
        struct tm *     tm_local;
        struct tm *     tm_utc;
        //<private>
        // time 函数返回的是 gmt 时间。  
        //</private>
        time( & t1 );
        //<private>
        // 用 localtime 取得本地时间后，再重新 mktime 成 time_t，这是本地时间。  
        //</private>
        t2                              = t1;
        tm_local                        = localtime( & t1 );
        t1                              = mktime( tm_local );
        //<private>
        // 用 gmtime 取得 gmt 时间后，再重新 mktime 成 time_t，这是 gmt 时间。  
        //</private>
        tm_utc                          = gmtime( & t2 );
        t2                              = mktime( tm_utc );
        //<private>
        // 计算 localtime 和 gmtime 的时间差（以小时为单位）。  
        //</private>
        g_dpr_datetime_timezone         = (int)( (t1 - t2) / 3600 );
#else
        time_t          t1;
        struct tm *     tm_local;
        //<private>
        // time 函数返回的是 gmt 时间。  
        //</private>
        time( & t1 );
        //<private>
        // 用 localtime 取得本地时间后，再重新 mktime 成 time_t，这是本地时间。  
        //</private>
        tm_local                        = localtime( & t1 );
        //<private>
        // 计算 localtime 和 gmtime 的时间差（以小时为单位）。  
        //</private>
        g_dpr_datetime_timezone         = (int)( tm_local->tm_gmtoff / 3600 );
#endif
        g_dpr_datetime_timezone_valid   = 1;
    }

    return g_dpr_datetime_timezone;
}

static inline void dpr_adjust_timezone( time_t * p, bool is_from_local )
{
    int tz = dpr_timezone();

    if ( is_from_local ) {
        // local -> gmt
        if ( tz > 0 ) {
            * p -= ( (time_t)tz * 60 * 60 );
        } else if ( tz < 0 ) {
            * p += ( (time_t)abs64( tz ) * 60 * 60 );
        }
    } else {
        // gmt -> local
        if ( tz > 0 ) {
            * p += ( (time_t)tz * 60 * 60 );
        } else if ( tz < 0 ) {
            * p -= ( (time_t)abs64( tz ) * 60 * 60 );
        }
    }
}

//<private>
// 每个月的天数  非闰年  
//</private>
static const char datetime_mon_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static inline struct tm * dpr_time_2_tm( const time_t *srctime, struct tm *tm_time, BOOL is_local )
{
    long int n32_Pass4year,n32_hpery;

    //<private>
    // 一年的小时数  365 * 24 (非闰年)  
    //</private>
    const static int ONE_YEAR_HOURS = 8760;

    //<private>
    //计算时差  
    //</private>
    time_t time;
    if ( unlikely( NULL == srctime || * srctime < 0 ) ) {
        if ( tm_time ) {
            memset( tm_time, 0, sizeof(struct tm) );
        }
        return NULL;
    }

    time            = * srctime;
    if ( is_local ) {
        dpr_adjust_timezone( & time, false );
    }
    tm_time->tm_isdst   = 0;
    if( time < 0 ) {
        time = 0;
    }

    //<private>
    //取秒时间  
    //</private>
    tm_time->tm_sec     =(int)(time % 60);
    time               /= 60;

    //<private>
    //取分钟时间  
    //</private>
    tm_time->tm_min     =(int)(time % 60);
    time               /= 60;

    //<private>
    //计算星期  
    //</private>
    tm_time->tm_wday    = ( time / 24 + 4 ) % 7;

    //<private>
    //取过去多少个四年，每四年有 1461*24 小时  
    //</private>
    n32_Pass4year       = ( (unsigned int)time / ( 1461L * 24L ) );

    //<private>
    //计算年份  
    //</private>
    tm_time->tm_year    = ( n32_Pass4year << 2 ) + 70;

    //<private>
    //四年中剩下的小时数  
    //</private>
    time                %= 1461L * 24L;

    //<private>
    //计算在这一年的天数  
    //</private>
    tm_time->tm_yday    = ( time / 24 ) % 365;

    //<private>
    //校正闰年影响的年份，计算一年中剩下的小时数  
    //</private>
    while ( 1 ) {
        //<private>
        //一年的小时数  
        //</private>
        n32_hpery       = ONE_YEAR_HOURS;

        //<private>
        //判断闰年  
        //</private>
        if ( ( tm_time->tm_year & 3 ) == 0 ) {
            //<private>
            //是闰年，一年则多24小时，即一天  
            //</private>
            n32_hpery  += 24;
        }

        if ( time < n32_hpery ) {
            break;
        }

        ++ tm_time->tm_year;
        time           -= n32_hpery;
    }

    //<private>
    //小时数  
    //</private>
    tm_time->tm_hour    =(int)( time % 24 );

    //<private>
    //一年中剩下的天数  
    //</private>
    time               /= 24;

    //<private>
    //假定为闰年  
    //</private>
    ++ time;

    //<private>
    //校正润年的误差，计算月份，日期  
    //</private>
    if ((tm_time->tm_year & 3) == 0) {
        if (time > 60) {
            -- time;
        } else {
            if (time == 60) {
                tm_time->tm_mon = 1;
                tm_time->tm_mday = 29;
                return tm_time;
            }
        }
    }

    //<private>
    //计算月日  
    //</private>
    for ( tm_time->tm_mon = 0;
          datetime_mon_days[ tm_time->tm_mon ] < time;
          ++ tm_time->tm_mon
    ) {
        time -= datetime_mon_days[tm_time->tm_mon];
    }

    tm_time->tm_mday = (int)(time);
    return tm_time;
}

static inline struct tm * dpr_localtime_r( const time_t * srctime, struct tm * tm_time )
{
    return dpr_time_2_tm( srctime, tm_time, TRUE );
}

static inline struct tm * dpr_gmttime_r( const time_t * srctime, struct tm * tm_time )
{
    return dpr_time_2_tm( srctime, tm_time, FALSE );
}

static inline time_t mktime2_inner(
    unsigned int  year,
    unsigned int  mon,
    unsigned int  day,
    unsigned int  hour,
    unsigned int  min,
    unsigned int  sec
)
{
    if ( unlikely( year < 1970 ) ) {
        return 0;
    } else if ( unlikely( 1970 == year ) ) {
        //<private>
        // 1970-1-1 0:0:0
        // mktime on windows return -1
        //        on linux   return 0
        //</private>
        if ( unlikely( mon <= 1 && day <= 1 && 0 == hour && 0 == min && 0 == sec ) ) {
            return 0;
        }
    }

    if ( 0 >= (int)(mon -= 2) ) {
        // 1..12 -> 11,12,1..10
        // Puts Feb last since it has leap day
        mon  += 12;
        year -= 1;
    }
    return (((
        (time_t) ( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) + year * 365 - 719499
        ) * 24 + hour   // hours
      ) * 60 + min      // minutes
    ) * 60 + sec;       // seconds
}

static inline time_t mktime2( const struct tm * ltm, BOOL tm_is_local )
{
    time_t r = mktime2_inner( ltm->tm_year + 1900,
                              ltm->tm_mon + 1,
                              ltm->tm_mday,
                              ltm->tm_hour,
                              ltm->tm_min,
                              ltm->tm_sec );
    if ( r > 0 ) {
        if ( tm_is_local ) {
            dpr_adjust_timezone( & r, true );
            if ( unlikely( r <= 0 ) ) {
                r = 0;
            }
        }
    }
    return r;
}

static inline time_t mktime_from_local( const struct tm * ltm )
{
    return mktime2( ltm, TRUE );
}

static inline time_t mktime_from_gmt( const struct tm * ltm )
{
    return mktime2( ltm, FALSE );
}

#ifdef mktime
    #undef mktime
#endif
#define mktime  mktime_from_local

//<private>
// ftello
// fseeko
// atoi64
// abs64
// atoll
// stricmp
// strnicmp
// snprintf
// strupr
// S_CRLF
//</private>
#if defined( _WIN32 ) || defined( _WIN64 )
    #define ftello          _ftelli64
    #define fseeko          _fseeki64
    #define atoi64(x)       _atoi64((x))
    #define atoll(x)        _atoi64((x))
    #define abs64(x)        _abs64((x))
    #ifndef stricmp
        #define stricmp     _stricmp
    #endif
    #ifndef strnicmp
        #define strnicmp    _strnicmp
    #endif
    #ifndef snprintf
        #define snprintf    _snprintf
    #endif
    #define lstrnicmp       _strnicmp
    #define S_CRLF          "\r\n"
    #define localtime_r( NOW, RET )     ((0 == localtime_s( (RET), (NOW) )) ? (RET) : NULL)
    #define gmttime_r( NOW, RET )       ((0 == gmttime_s( (RET), (NOW) )) ? (RET) : NULL)

#else
    //<non_windows>

    #define atoi64(x)       atoll((x))
    #define abs64(x)        llabs((x))
    #ifndef stricmp
        #define stricmp     strcasecmp
    #endif
    #define strnicmp        strncasecmp
    #define MAX_PATH        4096
    #define S_CRLF          "\n"

    static inline char * strupr( char * string )
    {
        if ( string ) {
            char *cp;       /* tracerses string for C locale conversion */
            for ( cp = string ; *cp ; ++cp )
                if ( ('a' <= *cp) && (*cp <= 'z') )
                    *cp -= 'a' - 'A';
            return(string);
        }
        return string;
    }

    //#if defined( __linux )
        #define localtime_r     dpr_localtime_r
        #define gmttime_r       dpr_gmttime_r
    //#endif // #if defined( __linux )

    //</non_windows>

#endif

//<private>
/*
void safe_snprintf(
         BOOL *          is_overflow,
         int *           written_size,
         char *          str,
         size_t          str_size,
         const char *    format,
         ...
);

int main( int argc, char ** argv )
{
    char s[ 2 ];
    char s2[ 1 ];
    int r;
    BOOL is_overflow;
    safe_snprintf( & is_overflow, & r, s, sizeof(s), "%d", 1 );
    assert( 0 == is_overflow && 1 == r && '1' == s[0] && '\0' == s[1] );

    safe_snprintf( & is_overflow, & r, s, sizeof(s), "%d", 11 );
    assert( 1 == is_overflow && 1 == r && '1' == s[0] && '\0' == s[1] );

    safe_snprintf( & is_overflow, & r, s2, sizeof(s2), "%d", 11 );
    assert( 1 == is_overflow && 0 == r && '\0' == s2[0] );

    return 0;
}
*/
//</private>
#define safe_snprintf( IS_OVERFLOW, WRITTEN_SIZE, STR, STR_SIZE, FORMAT, ... )          \
    if ( (STR_SIZE) > 0 ) {                                                             \
        *(WRITTEN_SIZE) = snprintf( (STR), (STR_SIZE), (FORMAT), ##__VA_ARGS__ );       \
        if ( unlikely( *(WRITTEN_SIZE) >= (int)(STR_SIZE) || *(WRITTEN_SIZE) < 0 ) ) {  \
            (STR)[ (STR_SIZE) - 1 ] = '\0';                                             \
            *(WRITTEN_SIZE) = (int)(STR_SIZE) - 1;                                      \
            *(IS_OVERFLOW)  = TRUE;                                                     \
        } else {                                                                        \
            *(IS_OVERFLOW)  = FALSE;                                                    \
        }                                                                               \
    } else if ( unlikely( (STR_SIZE) > 0 ) ) {                                          \
        *(IS_OVERFLOW)  = TRUE;                                                         \
        *(WRITTEN_SIZE) = 0;                                                            \
    } else {                                                                            \
        *(IS_OVERFLOW)  = FALSE;                                                        \
        *(WRITTEN_SIZE) = 0;                                                            \
    }

//<private>
// COUNT_OF
//</private>
#define COUNT_OF(a) (sizeof(a)/sizeof((a)[0]))

//<private>
// I64D
// I64U
//</private>
#ifdef _MSC_VER
    #ifndef I64D
        #define I64D "%I64d"
    #endif
    #ifndef I64U
        #define I64U "%I64u"
    #endif
    #ifndef PRIu64
        #define PRIu64 "I64u"
    #endif
#elif defined( __linux )
    #ifndef I64D
        #define I64D "%jd"
    #endif
    #ifndef I64U
        #define I64U "%ju"
    #endif
    #ifndef PRIu64
        #define PRIu64 "ju"
    #endif
#else
    #ifndef I64D
        #define I64D "%lld"
    #endif
    #ifndef I64U
        #define I64U "%llu"
    #endif
    #ifndef PRIu64
        #define PRIu64 "llu"
    #endif
#endif

// build on libmseed
#define __func__    __FUNCTION__

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

//<private>
// MSG_NO_SIGNAL
//</private>
#ifndef MSG_NOSIGNAL
    #define	MSG_NOSIGNAL		0
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
#if ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
    struct objc_object;
    typedef struct objc_object * id;
#endif
        
//<private>
    // getpid, getppid, gettid, getcwd
    // SOCKET
    // SOCKET_ERROR
    // INVALID_SOCKET
//</private>
#if defined( _WIN32 ) || defined( _WIN64 )

    typedef int                 pid_t;

    static inline char * getcwd( char * buf, int buf_len )
    {
        DWORD   r;

        if ( NULL == buf ) {
            return NULL;
        }

        if ( buf_len <= 1 ) {
            * buf = '\0';
            return NULL;
        }
        buf[ buf_len - 1 ] = '\0';            
        r = GetCurrentDirectoryA( (DWORD)buf_len, buf );
        if ( 0 == r ) {
            * buf = '\0';
            return NULL;
        }
        if ( buf[ buf_len - 1 ] != '\0' ) {
            * buf = '\0';
            return NULL;
        }

        return buf;
    }

    static inline pid_t getpid()
    {
        return (pid_t)GetProcessId( GetCurrentProcess() );
    }
    
    static inline int gettid()
    {
        return (int)GetCurrentThreadId();
    }

    static inline int getppid()
    {
        DWORD           pid     = GetProcessId( GetCurrentProcess() );
        DWORD           ppid    = -1;
        PROCESSENTRY32  pe;
        HANDLE          h;
        h = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        memset( & pe, 0, sizeof( PROCESSENTRY32 ) );
        pe.dwSize = sizeof( PROCESSENTRY32 );
        if ( Process32First( h, & pe ) ) {
            do {
                if ( pe.th32ProcessID == pid ) {
                    ppid = pe.th32ParentProcessID;
                    break;
                }
            } while( Process32Next( h, & pe ) );
        }
        CloseHandle( h );
        return (int)ppid;
    }
#else

    //<non_windows>

	typedef int	SOCKET;

    #ifndef SOCKET_ERROR
        #define SOCKET_ERROR    (int)-1
    #endif
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET  (int)-1
    #endif

    #if defined( __ANDROID__ )

//<private>
        // android has gettid function
//</private>

    #elif defined( __linux )
 
        static inline int gettid()
        {
            return syscall(__NR_gettid);
        }
    
    #elif defined( __APPLE__ )
    
        typedef __darwin_pid_t  pid_t;

        #define gettid()  pthread_mach_thread_np( pthread_self() )

    #elif defined( __FreeBSD__ )

        static inline int gettid()
        {
            void * p = pthread_self();
            long r;
            memcpy( & r, & p, sizeof( long ) );
            return (int)r;
        }

    #endif
    //</non_windows>
    
#endif
    
//<private>
    // socklen_t
//</private>
#if defined( _WIN32 ) || defined( _WIN64 ) || defined(__osf__)
    typedef int socklen_t;
#endif
    
    //<non_windows>
//<private>
    // TCHAR
//</private>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
	typedef char				TCHAR;
#endif
    //</non_windows>

#if ! defined( BYTE_DEFINED )
    #define BYTE_DEFINED    1
    //<private>
    // byte_t
    //</private>
    typedef unsigned char           byte_t;
#endif
    
//<private>
    // int16_t
    // uint16_t
//</private>
    typedef short                   int16_t;
    typedef unsigned short          uint16_t;
    
//<private>
    // S_64
    // int32_t
    // uint32_t
    // uint64_t
    // int64_t
//</private>
#if defined(__APPLE__) || defined(__FreeBSD__)
    //<non_windows>
    #define S_64    1
    #define S_32    0
    //</non_windows>

#elif ( defined( _WIN64 ) )
    
//<private>
    // 64 bit
//</private>
    typedef int                 int32_t;
    typedef unsigned int        uint32_t;
    
    typedef unsigned long long  uint64_t;
    typedef long long           int64_t;
    
    #define S_64    1
    #define S_32    0
    
#elif ( (defined(__sun) && defined(__sparcv9)) || (defined(__linux) && defined(__x86_64)) || (defined(__hppa) && defined(__LP64__)) || (defined(_ARCH_COM) && defined(__64BIT__)) )

    //<non_windows>
//<private>
    // 64 bit
//</private>
    typedef int                     int32_t;
    typedef unsigned int            uint32_t;
    
//<private>
    //typedef unsigned long long int  uint64_t;
    //typedef long long int           int64_t;
//</private>
    
    #define S_64    1
    #define S_32    0
    //</non_windows>
    
#else
    
//<private>
    // 32 bit
//</private>

    //<non_windows>
    #if ! defined(  __linux__ )
    //</non_windows>
        typedef int                 int32_t;
        typedef unsigned int        uint32_t;
    //<non_windows>
    #endif
    //</non_windows>

    //<non_windows>
    #if ! defined( __ANDROID__ )
    //</non_windows>
        typedef unsigned long long  uint64_t;
        typedef long long           int64_t;
    //<non_windows>
    #endif
    //</non_windows>

    #define S_64    0
    #define S_32    1
    
#endif
    
//<non_windows>
#if ! defined( _WIN32 ) && ! defined( _WIN32 )

typedef struct UUID
{
    unsigned char timeLow[4];
    unsigned char timeMid[2];
    unsigned char timeHighAndVersion[2];
    unsigned char clockSeqHiAndReserved;
    unsigned char clockSeqLow;
    unsigned char node[6];

} UUID;

#endif
//</non_windows>

//<private>
// __stdcall, __cdecl
//</private>
//<non_windows>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
    #if ( 0 == S_64 )
        #define __stdcall   __attribute__((__stdcall__))
        #define __cdecl     __attribute__((__cdecl__))
    #else
        #define __stdcall
        #define __cdecl
    #endif
#endif
//</non_windows>
    
//<private>
    // INT64( N )
//</private>
#if defined( _MSC_VER )
    #define INT64( n ) n##i64
#elif defined(__HP_aCC)
    #define INT64(n)    n
#elif S_64
    #define INT64(n)    n##L
#else
    #define INT64(n)    n##LL
#endif
    
//<non_windows>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
    #define UINT32  uint32_t
    #define UINT16  uint16_t
    #define LONG    long
#endif
//</non_windows>

//<private>
/**
 * @brief 求指定积的，以指定数字为底的对数，它是乘方的反操作。  
 * @note  uint32_t dpr_log( uint32_t pow_value, uint32_t base )
 * @warning 为了性能，这里实现。本来函数名叫 log2的，结果linux下有重名函数。  
 * @param[in] uint32_t pow_value 乘方值  
 * @param[in] uint32_t base      底  
 * @return 对数  
 */
//</private>
static inline uint32_t dpr_log( uint32_t pow_value, uint32_t base )
{
    assert( base > 0 );
    return (uint32_t)( log( (double)(pow_value) ) / log( (double)(base) ) );
}

#ifdef __cplusplus
}
#endif

#endif // #ifndef _GROCKET_INCLUDE_GRSTDINC_H_

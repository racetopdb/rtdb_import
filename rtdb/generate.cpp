#include "wide_base.h"
#include <assert.h>

#ifdef _DEBUG
    #define DEFAULT_BOOL_FIELD_COUNT    1
    #define DEFAULT_INT_FIELD_COUNT     1
    #define DEFAULT_FLOAT_FIELD_COUNT   1
    #define DEFAULT_TOTAL_FIELD_COUNT   3
#else
    #define DEFAULT_BOOL_FIELD_COUNT    200000
    #define DEFAULT_INT_FIELD_COUNT     200000
    #define DEFAULT_FLOAT_FIELD_COUNT   200000
    #define DEFAULT_TOTAL_FIELD_COUNT   100000
#endif

namespace rtdb
{

namespace wide
{

int generate_table( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    TSDB_INFO( p, "[GENERATE]Generate Test Data" );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    // -format txt
    // -format sql
    // output file format
    const char * format = NULL;
    p->tools->find_argv( argc, argv, "format", & format, NULL );
    if ( NULL == format || '\0' == * format ) {
        format = "txt";
    } else if ( 0 != stricmp( format, "txt" ) && 0 != stricmp( format, "sql" ) ) {
        TSDB_ERROR( p, "[GENERATE][PARAMETERS][format=%s]invalid format", format );
        return EINVAL;
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][format      =%s]", format );

    // -dst path
    // destination file path. we will auto create directory, if needed.
    std::string dst;
    {
        const char * _dst = NULL;
        p->tools->find_argv( argc, argv, "dst", & _dst, NULL );
        if ( NULL == _dst || '\0' == * _dst ) {
            if ( 0 == stricmp( "sql", format ) ) {
                _dst = "./generate_table.sql";
            } else {
                _dst = "./generate_table.txt";
            }
        }
        dst = _dst;
        p->tools->path_to_os( & dst[0] );
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][dst         =%s]", dst.c_str() );

    // Database name, this will be write into destination file.
    const char * db = NULL;
    p->tools->find_argv( argc, argv, "db", & db, NULL );
    if ( NULL == db || '\0' == * db ) {
        db = "DB_NARROW";
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][db          =%s]", db );

    // table name leadding string
    const char * table_lead = NULL;
    p->tools->find_argv( argc, argv, "table_lead", & table_lead, NULL );
    if ( NULL == table_lead || '\0' == * table_lead ) {
        table_lead = "TABLE_";
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][table_lead  =%s]", table_lead );

    // -bool 200000
    // -bool 0           create bool field, default count is 200000.
    // -bool -1          don't create bool field
    // how much bool field count 
    int points_bool = 0;
    p->tools->find_argv_int( argc, argv, "bool", & points_bool );
    if ( points_bool == 0 ) {
        points_bool = DEFAULT_BOOL_FIELD_COUNT;
    } else if ( points_bool < 0 ) {
        points_bool = 0;
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][points_bool =%d]", points_bool );

    // -float 200000
    // how much float field count 
    // at least, all field is 100000
    int points_float = 0;
    p->tools->find_argv_int( argc, argv, "float", & points_float );
    if ( points_float == 0 ) {
        points_float = DEFAULT_FLOAT_FIELD_COUNT;
    } else if ( points_float < 0 ) {
        points_float = 0;
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][points_float  =%d]", points_float );

    // -int 200000
    // -int 0           create int field, default count is 200000.
    // -int -1          don't create int field
    // how much int field count 
    int points_int = 0;
    p->tools->find_argv_int( argc, argv, "int", & points_int );
    if ( points_int == 0 ) {
        points_int = DEFAULT_INT_FIELD_COUNT;
    } else if ( points_int < 0 ) {
        points_int = 0;
    }

    if ( (int)(points_bool + points_int + points_float) < DEFAULT_TOTAL_FIELD_COUNT ) {
        // at least 100000
        points_int = DEFAULT_TOTAL_FIELD_COUNT - points_bool - points_int;
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][points_int=%d]", points_float );

    // open destination file, create directory if needed.
    FILE * fp = fopen( dst.c_str(), "wb" );
    if ( NULL == fp ) {
        char * s = strrchr( (char *)dst.c_str(), S_PATH_SEP_C );
        if ( p ) {
            * s = '\0';
            p->tools->make_dir( dst.c_str() );
            fp = fopen( dst.c_str(), "wb" );
            * s = S_PATH_SEP_C;
        }
        if ( NULL == fp ) {
            TSDB_ERROR( p, "[GENERATE][path=%s]fopen failed", dst.c_str() );
            return EIO;
        }
    }

    if ( 0 == stricmp( "txt", format ) ) {

        // output Text info into file.

        TSDB_INFO( p, "[GENERATE][path=%s][format=TXT]", dst.c_str() );

        fprintf( fp, "#DB,%s" S_CRLF, db );
        for ( size_t i = 0; i < (size_t)points_bool; ++ i ) {
            fprintf( fp, "#TABLE,%sbool_%d,bool" S_CRLF, table_lead, (int)i );
        }
        for ( size_t i = 0; i < (size_t)points_int; ++ i ) {
            fprintf( fp, "#TABLE,%sint_%d,int" S_CRLF, table_lead, (int)i );
        }
        for ( size_t i = 0; i < (size_t)points_float; ++ i ) {
            fprintf( fp, "#TABLE,%sfloat_%d,float" S_CRLF, table_lead, (int)i );
        }

    } else if ( 0 == stricmp( "sql", format ) ) {

        // output SQL statment into file.

        TSDB_INFO( p, "[GENERATE][path=%s][format=SQL]", dst.c_str() );

        fprintf( fp, "create database if not exists %s with concurrent=40;" S_CRLF, db );
        fprintf( fp, "use %s;" S_CRLF, db );

        for ( size_t i = 0; i < (size_t)points_bool; ++ i ) {
            fprintf( fp, "create table if not exists %sbool_%d(time timestamp, v bool);" S_CRLF, table_lead, (int)i );
        }
        for ( size_t i = 0; i < (size_t)points_int; ++ i ) {
            fprintf( fp, "create table if not exists %sint_%d(time timestamp, v int);" S_CRLF, table_lead, (int)i );
        }
        for ( size_t i = 0; i < (size_t)points_float; ++ i ) {
            fprintf( fp, "create table if not exists %sfloat_%d(time timestamp, v float);" S_CRLF, table_lead, (int)i );
        }
    } else {
        TSDB_ERROR( p, "[GENERATE][path=%s][format=format]invalid format", dst.c_str() );
    }

    fclose( fp );

    TSDB_INFO( p, "[GENERATE][path=%s][bool=%d][int=%d][float=%d][total=%d][file_size=%lld] file generated.",
                  dst.c_str(), (int)points_bool, (int)points_int, (int)points_float,
                  (int)(points_bool+points_int+points_float),
                  (long long)p->tools->get_file_size(dst.c_str())
    );

#if defined( _WIN32 )
    {
        // Windows, We Open the file explorer, and explorer to the destination directory.

        std::string t = dst;
        char * s = (char*)strrchr( t.c_str(), S_PATH_SEP_C );
        if ( s ) * s = '\0';

        std::string cmd;
        cmd = "explorer.exe ";
        cmd += t;
        WinExec( cmd.c_str(), SW_SHOW );
    }
#else
    //TODO:
#endif
    return 0;
}

} // namespace wide

} // namespace rtdb


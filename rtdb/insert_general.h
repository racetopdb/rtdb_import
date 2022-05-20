#ifndef _rtdb_test_wide_insert_general_h_
#define _rtdb_test_wide_insert_general_h_

#include "wide_base.h"
//#include "source/source.h"
#include <map>


namespace rtdb
{

namespace wide
{

class import_source_t;


// wide table, each table just only has one field.
struct thread_param_insert_table_general_t
{

    thread_param_insert_table_general_t()

        : lines()

    {}



    // Database Engine type

    db_type_t                   engine;



    wide_conn_t* conn;



    // Thread index [0, thread_count)

    uint32_t                    thread_id;

    // Thread Count. by default, this value same with CPU core count.

    uint32_t                    thread_count;



    // single SQL max bytes

    uint32_t                    sql_bytes;



    // first time by ms

    // in writing progress, the value updated to the time when the data is currently written.

    volatile uint64_t           start_time;

    // stop time

    uint64_t                    stop_time;

    // stop on local time

    uint64_t                    stop_local_time;

    // stop line in current thread

    uint64_t                    stop_line;

    // step time by ms

    uint32_t                    step_time;



    // realtime insert table count in this thread.

    volatile uint64_t           insert_line_count;

    // We need insert table count in this thread.

    uint32_t                    insert_need;

    // Insert table from index

    uint32_t                    insert_from;

    // error code, 0 indicate OK, error otherwise.

    int                         r;



    // point to line data array.

    std::vector< tsdb_str >* lines;

    // database name

    const char* db;


    // thread object

    pthread_t                   thread;

    // Is current thread already exited? 

    // current thread need set this value to true before quit the thread.

    volatile bool               exited;

    // source 集合  
    std::vector<import_source_t*> *vt_import_source_t;

    // padding memory, nothing.
    // 插入点数  
    volatile uint64_t           insert_point;

    char                        padding[64];

};

int insert_table_general( int argc, char ** argv );



} // namespace wide

} // namespace rtdb

#endif

#include "wide_base.h"
#include <assert.h>
#include <map>
#include "utils.h"


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

struct thread_param_generate_general_t
{
    // Thread index [0, thread_count)
    uint32_t                    thread_id;
        
    // Thread Count. by default, this value same with CPU core count.
    uint32_t                    thread_count;

    // realtime created data count in this thread.
    volatile uint32_t           create_count;
        
    // We need create data count in this thread.
    uint32_t                    create_need;
        
    // Create table from index
    uint32_t                    create_from;

    // 步长间隔  
    int                         step_time;

    // error code, 0 indicate OK, error otherwise.
    int                         r;

    // 起始时间  
    uint64_t start_time;

    // 结束时间  
    uint64_t stop_time;

    // 每个线程需要创建的数据条数  
    int line_count_per_table;

    // 当前创建了多少条数据了 单指的是一个表  
    int line_current_per_table;
        
    // thread object
    pthread_t                   thread;
        
    // Is current thread already exited? 
    // current thread need set this value to true before quit the thread.
    volatile bool               exited;

    // 表配置信息  
    std::map<std::string, struct test_table_file_info_t>* map_test_table_file_info_t;

    // 表配置信息 vector  
    std::vector<struct test_table_file_info_t*>  *vt_test_table_file_info_t;
    

    // 线程内运行时信息  
    std::vector<struct generate_table_data_runtime_info_t>  *vt_generate_table_data_runtime_info_t;;

    // 当前创建了多少条数据了  
    int current_count;

    // padding memory, nothing.
    char                        padding[64];
};


void* generate_data_general_thread(void* _param)
{
    thread_param_generate_general_t* param = (thread_param_generate_general_t*)_param;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    // calc the table count that We need to create. 
    param->create_need = (uint32_t)(param->vt_test_table_file_info_t->size() / (size_t)param->thread_count);
    // calc which index should I start with.
    param->create_from = param->create_need * param->thread_id;
    if (param->thread_id == param->thread_count - 1) {
        // in last thread, we need add the remainder.
        param->create_need += (uint32_t)(param->vt_test_table_file_info_t->size() % (size_t)param->thread_count);
    }
   
    param->current_count = 0;
    // create tables data  
    for (param->line_current_per_table = 0; param->line_current_per_table < param->line_count_per_table;  param->line_current_per_table++) {
        for (param->create_count = 0; param->create_count < param->create_need; ++param->create_count) {

            // calc the real index in global array.
            size_t i = param->create_count + param->create_from;
            // get item
            struct test_table_file_info_t *&ttfi = param->vt_test_table_file_info_t->at(i);
            std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t = ttfi->vt_test_tb_field_info_t;
            std::string & field_list_file = ttfi->field_list_file;
            std::vector<struct generate_table_data_runtime_info_t>  *&vt_generate_table_data_runtime_info_t =  
                param->vt_generate_table_data_runtime_info_t;

            struct generate_table_data_runtime_info_t& gtdri = (*vt_generate_table_data_runtime_info_t)[i];
            
            std::string &file_path = gtdri.file_path;
            file_operation &fo = gtdri.fo;

            // 是否已经打开  
            if (!gtdri.is_file_open) {
                file_path = field_list_file;
                file_path += ".data";
                p->tools->path_to_os(&file_path[0]);
                param->r = fo.open_by_write(file_path.c_str());
                if (0 != param->r) {
                    TSDB_ERROR(p, "[GENERATE][path:%s] open_by_write failed", file_path.c_str());
                    param->exited = true;
                    return NULL;
                }
                gtdri.is_file_open = true;
            }

            std::string s;

            // 首次  
            if (0 == param->line_current_per_table) {

                // 初始化和字段个数相同  
                gtdri.vt_field_increase_store_t.resize(vt_test_tb_field_info_t.size());
                param->r =  init_data_for_batch_by_default(
                    gtdri.vt_field_increase_store_t,
                    vt_test_tb_field_info_t,
                    param->start_time,
                    (float)param->step_time);
                if (0 != param->r) {
                    TSDB_ERROR(p, "[GENERATE][path:%s] init_data_for_batch_by_default failed", file_path.c_str());
                    param->exited = true;
                    return NULL;
                }
                // 忽略 i=0 时间主键 插入字段名字  
                generate_field_for_one_line(vt_test_tb_field_info_t, 1,"\t", s);
                s += '\n';

                int len = fo.write_one_line(s.c_str(), (unsigned int)s.length(), true);
                if (len != (int)s.length()) {
                    param->r = EFAULT;
                    TSDB_ERROR(p, "[GENERATE][path:%s][line:%d] write_one_line failed", file_path.c_str(), param->line_current_per_table);
                    param->exited = true;
                    return NULL;
                }
                param->current_count++;
            }

            s.resize(0);
            // 忽略 i=0 时间主键 插入数据  
            param->r = generate_data_for_one_line(gtdri.vt_field_increase_store_t,
                vt_test_tb_field_info_t, 1, "\t", s);
            if (0 != param->r) {
                param->r = EFAULT;
                TSDB_ERROR(p, "[GENERATE][path:%s][line:%s] generate_data_for_one_line failed", file_path.c_str(), s.c_str());
                param->exited = true;
                return NULL;
            }

            s += '\n';
            int len = fo.write_one_line(s.c_str(), (unsigned int)s.length(), true);
            if (len != (int)s.length()) {
                param->r = EFAULT;
                TSDB_ERROR(p, "[GENERATE][path:%s][line:%s] write_one_line failed", file_path.c_str(), s.c_str());
                param->exited = true;
                return NULL;
            }
            param->current_count++;
        }
    }

    // 关闭全部文件  
    for (param->create_count = 0; param->create_count < param->create_need; ++param->create_count) {

        // calc the real index in global array.
        size_t i = param->create_count + param->create_from;
        // get item
        struct generate_table_data_runtime_info_t& gtdri = (*param->vt_generate_table_data_runtime_info_t)[i];
        file_operation& fo = gtdri.fo;
        if (gtdri.is_file_open) {
            fo.close();
        }
    }

    // quit the thread
    param->exited = true;
    return NULL;
}

int generate_data_general( int argc, char ** argv )
{
    int r = 0;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    TSDB_INFO( p, "[GENERATE]Generate Import Data" );

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

    // -path path
    // destination file path. we will auto create directory, if needed.
    std::string path;
    {
        const char * _path = NULL;
        p->tools->find_argv( argc, argv, "path", & _path, NULL );
        if ( NULL == _path || '\0' == * _path ) {
            TSDB_ERROR(p, "[GENERATE][PARAMETERS]path not found");
            return EINVAL;
        }
        path = _path;
        p->tools->path_to_os( & path[0] );
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][path         =%s]", path.c_str() );


    // create table thread count
    int thread_count;
    p->tools->find_argv_int(argc, argv, "thread", &thread_count);
    if (thread_count <= 0) {
        thread_count = p->tools->get_cpu_core_count();
    }
    TSDB_INFO(p, "[GENERATE][PARAMETERS][thread      =%d]", thread_count);



    // Database name, this will be write into destination file.
    const char * db = NULL;
    p->tools->find_argv( argc, argv, "db", & db, NULL );
    if ( NULL == db || '\0' == * db ) {
        db = "DB_TEST_WRITE";
    }
    TSDB_INFO( p, "[GENERATE][PARAMETERS][db          =%s]", db );



    // find table start time
    uint64_t start_time = 0;
    p->tools->find_argv_datetime(argc, argv, "start_time", &start_time);
    if (0 == start_time) {
        TSDB_ERROR(p, "[GENERATE][PARAMETERS] invalid start_time");
        return EINVAL;
    }
    char start_time_s[64];
    int  start_time_sl = (int)sizeof(start_time_s);
    p->tools->datetime_to_str(start_time, start_time_s, &start_time_sl);
    TSDB_INFO(p, "[GENERATE][PARAMETERS][start_time  =%lld, %s]", start_time, start_time_s);



    // find table stop time  
    uint64_t stop_time = 0;
    p->tools->find_argv_datetime(argc, argv, "stop_time", &stop_time);
    // 忽略没有参数情况  
   

    char stop_time_s[64] = {0};
    int  stop_time_sl = (int)sizeof(stop_time_s);
    if (0 != stop_time) {
        p->tools->datetime_to_str(stop_time, stop_time_s, &stop_time_sl);
    }
    TSDB_INFO(p, "[GENERATE][PARAMETERS][stop_time  =%lld, %s]", stop_time, stop_time_s);


    int step_time = 0;
    p->tools->find_argv_int(argc, argv, "step_time", &step_time);
    if (step_time == 0) {
        step_time = 1;
    }
    TSDB_INFO(p, "[GENERATE][PARAMETERS][step_time =%d]", step_time);

    int line_count = 0;
    p->tools->find_argv_int(argc, argv, "line_count", &line_count);
    TSDB_INFO(p, "[GENERATE][PARAMETERS][line_count =%d]", line_count);

    // 行数 和 停止时间 不能同时没有  
    if (0 == line_count && 0 == stop_time) {
        TSDB_ERROR(p, "[GENERATE][PARAMETERS] 'line_count' and 'stop_time' not found");
        return EINVAL;
    }

    if (0 != line_count) {
        // 计算真实的stop_time line_count 与当前的stop时间进行对比 取小值  
        uint64_t stop_time_tmp = start_time + (uint64_t)line_count * step_time;
        if (0 == stop_time) {
            stop_time = stop_time_tmp;
        }
        else {
            if (stop_time_tmp < stop_time) {
                stop_time = stop_time_tmp;
            }
        }
        // 重新计算结束时间  
        memset(stop_time_s, 0x00, sizeof(stop_time_s));
        stop_time_sl = (int)sizeof(stop_time_s);
        if (0 != stop_time) {
            p->tools->datetime_to_str(stop_time, stop_time_s, &stop_time_sl);
        }
        TSDB_INFO(p, "[GENERATE][PARAMETERS][start_time  =%lld, %s]", stop_time, stop_time_s);
    }
    
    //  重新计算 line_count  
    line_count = (int)((stop_time - start_time) / step_time);
   

    // 解析文件中的内容  
    std::map<std::string, struct test_table_file_info_t> map_test_table_file_info_t;
    r = parse_table_conf_file(path.c_str(), map_test_table_file_info_t);
    if (0 != r) {
        TSDB_INFO(p, "[GENERATE][path=%s] parse_table_conf_file failed", path.c_str());
        return r;
    }
    std::vector<struct test_table_file_info_t*>  vt_test_table_file_info_t;
    r = convert_table_conf_map_to_table_vector_ex(map_test_table_file_info_t, vt_test_table_file_info_t);
    if (0 != r) {
        TSDB_INFO(p, "[GENERATE][path=%s] convert_table_conf_map_to_table_vector_ex failed", path.c_str());
        return r;
    }

    // 产生数据运行时信息  
    std::vector<struct generate_table_data_runtime_info_t>  vt_generate_table_data_runtime_info_t;
    vt_generate_table_data_runtime_info_t.resize(vt_test_table_file_info_t.size());
    for (size_t i=0; i<vt_generate_table_data_runtime_info_t.size(); i++)
    {
        vt_generate_table_data_runtime_info_t[i].file_path = "";
        vt_generate_table_data_runtime_info_t[i].is_file_open = false;
        //vt_generate_table_data_runtime_info_t[i].fo;
        vt_generate_table_data_runtime_info_t[i].vt_field_increase_store_t.resize(0);
    }


    // prepare CREATE TABLE threads

    std::vector< thread_param_generate_general_t >   threads;
    threads.resize(thread_count);
    for (size_t i = 0; i < threads.size(); ++i) {
        thread_param_generate_general_t& item = threads[i];
       
        item.thread_id = (uint32_t)i;
        item.thread_count = (uint32_t)threads.size();
        item.create_count = 0;
        item.create_need = 0;
        item.create_from = 0;
        item.step_time = step_time;
        item.r = 0;
        item.start_time = start_time;
        item.stop_time = stop_time;
        item.line_count_per_table = line_count;
        item.line_current_per_table = 0;
        item.thread = 0;
        item.exited = false;
        item.map_test_table_file_info_t = &map_test_table_file_info_t;
        item.vt_test_table_file_info_t = &vt_test_table_file_info_t;
        item.vt_generate_table_data_runtime_info_t = &vt_generate_table_data_runtime_info_t;
        item.current_count = 0;
    }

    for (size_t i = 0; i < threads.size(); ++i) {
        thread_param_generate_general_t& item = threads[i];
        item.exited = false;
        if (unlikely(!p->tools->thread_start(&item.thread, generate_data_general_thread, &item, 0))) {
            TSDB_ERROR(p, "[GENERATE]thread_start failed");
            item.exited = true;
            return EFAULT;
        }
    }

    unsigned long start = p->tools->get_tick_count();
    unsigned long last_show = start;
    uint32_t      last_count = 0;

    while (true) {

        // sleep 100 ms
        p->tools->sleep_ms(100);

        unsigned long stop = p->tools->get_tick_count();
        unsigned long span = stop - last_show;
        if (unlikely(span >= 1000)) {

            // If last show create info time pass 1 s, then show create info now.

            // get current time as string, this for show to human.
            char s[64];
            int  sl = (int)sizeof(s);
            p->tools->datetime_to_str(p->tools->datetime_now(), s, &sl);

            // collect thread exit status, and create table count.
            uint32_t exit_count = 0;
            uint32_t count = 0;
            for (size_t i = 0; i < threads.size(); ++i) {
                thread_param_generate_general_t& item = threads[i];
                count += item.current_count;
                if (item.exited) {
                    ++exit_count;
                }
            }

            uint32_t speed = count - last_count;
            uint32_t rest;
            if (speed > 0) {
                // calculate the speed, be careful for div 0.
                rest = 0;
            }
            else {
                rest = 0;
            }

            fprintf(stderr, "\r%s [GENERATE][%d][%d / %d][speed=%d/s][rest=%d s]...",
                s,  (int)(vt_test_table_file_info_t.size()), (int)count, (int)((line_count+1)*vt_test_table_file_info_t.size()), speed, rest
            );

            // member this time
            last_show = stop;
            last_count = count;

            if (exit_count == (uint32_t)threads.size()) {
                // If all thread exited, then exit the loop now.
                break;
            }
        }
    }
    fprintf(stderr, "\n");

    unsigned long stop = p->tools->get_tick_count();
    unsigned long span = stop - start;
    TSDB_INFO(p, "[GENERATE][table_count=%d][use=%d ms]", (int)(vt_test_table_file_info_t.size()), span);

    // Close Thread
    for (size_t i = 0; i < threads.size(); ++i) {
        thread_param_generate_general_t& item = threads[i];
        p->tools->thread_join(&item.thread);
    }
    print_current_path();
#if defined( _WIN32 )
    {
        // Windows, We Open the file explorer, and explorer to the destination directory.

        std::string t = path;
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

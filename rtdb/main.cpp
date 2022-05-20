#include "main.h"
#include <sstream>

using namespace rtdb::wide;

static void usage( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    // get file name.
    std::string ph = argv[0] ? argv[0] : "";
    if ( ! ph.empty() ) {
        p->tools->path_to_os( & ph[0] );
        const char * s = strrchr( ph.c_str(), S_PATH_SEP_C );
        if ( s ) {
            ph = s + 1;
        }
    }

    std::stringstream ss;
    ss << S_CRLF S_CRLF;

    ss << "generate test data:" S_CRLF S_CRLF;
    ss << "\t" << ph << " -generate.data.general  -thread 2 -path ./general_table.conf -format txt  -start_time 2020-01-01 -step_time 1000 -stop_time 2020-02-01 -line_count 100"   << S_CRLF;
    ss << "\t\tthread:       how many threads used to create data, default thread count same with CPU core count" << S_CRLF;
    ss << "\t\tpath:         file path generate by 'general_table.conf' file." << S_CRLF;
    ss << "\t\tformat:       only support 'txt', default value is 'txt'" << S_CRLF;
    ss << "\t\tstart_time:   start time." S_CRLF;
    ss << "\t\tstep_time:    after writing data to all tables once, the timestamp adjusted to increment a fixed interval. This value is the fixed interval." << S_CRLF;
    ss << "\t\tstop_time:    stop time. " S_CRLF;
    ss << "\t\t              YYYY-MM-DD               stop date string." S_CRLF;
    ss << "\t\t              YYYY-MM-DD HH:mm:SS.000  stop time string." S_CRLF;
    ss << "\t\t              2s                       stop time after n seconds." S_CRLF;
    ss << "\t\t              2m                       stop time after n minutes." S_CRLF;
    ss << "\t\t              2h                       stop time after n hours." S_CRLF;
    ss << "\t\t              2d                       stop time after n days." S_CRLF;
    ss << "\t\t              2w                       stop time after n weeks." S_CRLF;
    ss << "\t\tline_count:   need create lines one tables. " S_CRLF;
    ss << "\t\t              line_count and stop_time cannot be 0 at the same time.." S_CRLF;
    ss << "\t\t              line_count priority is greater than stop_time ." S_CRLF;
    ss << S_CRLF;

    ss << "create table:" S_CRLF S_CRLF;
    ss << "\t" << ph << " -create.table.general -engine rtdb -server 127.0.0.1:9000 -thread 3 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path ./general_table.conf -format txt  -db DB_TEST_WRITE"        << S_CRLF;
#if ENABLE_TDENGINE
    ss << "\t" << ph << " -create.table.general -engine taos -server 192.168.1.43:6030 -thread 3 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path ./general_table.conf -format txt  -db DB_TEST_WRITE"        << S_CRLF;
#endif // #if ENABLE_TDENGINE
#if ENABLE_TIMESCALEDB
    ss << "\t" << ph << " -create.table.general -engine timescaledb -server 192.168.1.43:5432 -thread 3 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path ./general_table.conf -format txt  -db DB_TEST_WRITE" << S_CRLF;
#endif // #if ENABLE_TIMESCALEDB
    ss << "\t\tengine:       rtdb      RTDB (http://www.rtdb.com)." << S_CRLF;
#if ENABLE_TDENGINE
    ss << "\t\t              taos      TAOS (http://www.taosdata.com)." << S_CRLF;
#endif // #if ENABLE_TDENGINE
#if ENABLE_TIMESCALEDB
    ss << "\t\t              timescaledb      TIMESCALEDB (https://www.timescale.com/)." << S_CRLF;
#endif // #if ENABLE_TIMESCALEDB
    ss << "\t\tserver:       server address, format is 'ip:port'." << S_CRLF;
    ss << "\t\tthread:       how many threads used to create table, default thread count same with CPU core count" << S_CRLF;
    ss << "\t\ttimeout.conn: socket connect timeout by ms. '3000' by default." S_CRLF;
    ss << "\t\ttimeout.send: socket connect timeout by ms. 'infinite' by default." S_CRLF;
    ss << "\t\ttimeout.recv: socket connect timeout by ms. '3000' by default." S_CRLF;
    ss << "\t\tpath:         file path generate by 'general_table.conf' file." << S_CRLF;
    ss << "\t\tformat:       only support 'txt', default value is 'txt'" << S_CRLF;
    ss << "\t\tdb:           which database will be createdd. default value is 'DB_TEST_WRITE'" << S_CRLF;
    ss << S_CRLF;

    ss << "insert into table:" S_CRLF S_CRLF;
    ss << "\t" << ph << " -insert.table.general -engine rtdb -server 127.0.0.1:9000 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000 -stop_time 1m  -sql_size 128k -db DB_TEST_WRITE -table_conf ./general_table.conf"        << S_CRLF;
#if ENABLE_TDENGINE
    ss << "\t" << ph << " -insert.table.general -engine taos -server 192.168.1.43:6030 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000 -stop_time 1m -sql_size 128k -db DB_TEST_WRITE -table_conf ./general_table.conf"        << S_CRLF;
#endif // #if ENABLE_TDENGINE
#if ENABLE_TIMESCALEDB
    ss << "\t" << ph << " -insert.table.general -engine timescaledb -server 192.168.1.43:5432 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000 -stop_time 1m -sql_size 128k -db DB_TEST_WRITE -table_conf ./general_table.conf" << S_CRLF;
#endif // #if ENABLE_TIMESCALEDB
    ss << "\t\tengine:       rtdb      RTDB (http://www.rtdb.com)." << S_CRLF;
#if ENABLE_TDENGINE
    ss << "\t\t              taos      TAOS (http://www.taosdata.com)." << S_CRLF;
#endif // #if ENABLE_TDENGINE
#if ENABLE_TIMESCALEDB
    ss << "\t\t              timescaledb      TIMESCALEDB (https://www.timescale.com/)." << S_CRLF;
#endif // #if ENABLE_TIMESCALEDB
    ss << "\t\tserver:       server address, format is 'ip:port'." << S_CRLF;
    ss << "\t\tthread:       how many threads used to write data, default thread count same with CPU core count" << S_CRLF;
    ss << "\t\ttimeout.conn: socket connect timeout by ms. '3000' by default." S_CRLF;
    ss << "\t\ttimeout.send: socket connect timeout by ms. 'infinite' by default." S_CRLF;
    ss << "\t\ttimeout.recv: socket connect timeout by ms. '3000' by default." S_CRLF;
    ss << "\t\tstart_time:   start time." S_CRLF;
    ss << "\t\tstep_time:    after writing data to all tables once, the timestamp adjusted to increment a fixed interval. This value is the fixed interval." << S_CRLF;
    ss << "\t\t              unit is ms, default value is 1000, that means 1 second." << S_CRLF;
    ss << "\t\tstop_time:    stop time. path exists Automatically lapse" S_CRLF;
    ss << "\t\t              YYYY-MM-DD               stop date string." S_CRLF;
    ss << "\t\t              YYYY-MM-DD HH:mm:SS.000  stop time string." S_CRLF;
    ss << "\t\t              2s                       stop time after n seconds." S_CRLF;
    ss << "\t\t              2m                       stop time after n minutes." S_CRLF;
    ss << "\t\t              2h                       stop time after n hours." S_CRLF;
    ss << "\t\t              2d                       stop time after n days." S_CRLF;
    ss << "\t\t              2w                       stop time after n weeks." S_CRLF;
    ss << "\t\tsql_size:     max bytes of an SQL statement, number followed by a charactor Suggest 128K" << S_CRLF;
    ss << "\t\tpath:         file path generate by 'general_data.conf' file. if path not exist The mode is to generate data automatically!!!" << S_CRLF;
    ss << "\t\tformat:       only support 'txt', default value is 'txt'" << S_CRLF;
    ss << "\t\tdb:           which database will be createdd. default value is 'DB_TEST_WRITE'" << S_CRLF;
    ss << "\t\ttable_conf:   file path generate by 'general_table.conf' file." << S_CRLF;
    ss << S_CRLF;


    ss << "find from table:" S_CRLF S_CRLF;
    ss << "\t" << ph << " -insert.table.general -engine rtdb -server 127.0.0.1:9000 -thread 80 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 1m -stop_line 1000 -db DB_TEST_WRITE -path ./general_table.conf -format txt"        << S_CRLF;
#if ENABLE_TDENGINE
    ss << "\t" << ph << " -find.table.general -engine taos -server 127.0.0.1:6030 -thread 80 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 1m -stop_line 1000 -db DB_TEST_WRITE -path ./general_table.conf -format txt"        << S_CRLF;
#endif // #if ENABLE_TDENGINE
#if ENABLE_TIMESCALEDB
    ss << "\t" << ph << " -find.table.general -engine timescaledb -server 127.0.0.1:5432 -thread 80 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 1m -stop_line 1000 -db DB_TEST_WRITE -path ./general_table.conf -format txt" << S_CRLF;
#endif // #if ENABLE_TIMESCALEDB
    ss << "\t\tengine:       rtdb      RTDB (http://www.rtdb.com)." << S_CRLF;
#if ENABLE_TDENGINE
    ss << "\t\t              taos      TAOS (http://www.taosdata.com)." << S_CRLF;
#endif // #if ENABLE_TDENGINE
#if ENABLE_TIMESCALEDB
    ss << "\t\t              timescaledb      TIMESCALEDB (https://www.timescale.com/)." << S_CRLF;
#endif // #if ENABLE_TIMESCALEDB
    ss << "\t\tserver:       server address, format is 'ip:port'." << S_CRLF;
    ss << "\t\tthread:       how many threads used to find data, default thread count same with CPU core count" << S_CRLF;
    ss << "\t\ttimeout.recv: socket connect timeout by ms. '3000' by default." S_CRLF;
    ss << "\t\tstart_time:   start time." S_CRLF;
    ss << "\t\tstep_time:    after find data for all tables once, the timestamp adjusted to increment a fixed interval. This value is the fixed interval." << S_CRLF;
    ss << "\t\t              unit is ms, default value is 3600000, that means 1 hour." << S_CRLF;
    ss << "\t\tstop_time:    stop time.  default forever" S_CRLF;
    ss << "\t\t              YYYY-MM-DD               stop date string." S_CRLF;
    ss << "\t\t              YYYY-MM-DD HH:mm:SS.000  stop time string." S_CRLF;
    ss << "\t\t              2s                       stop time after n seconds." S_CRLF;
    ss << "\t\t              2m                       stop time after n minutes." S_CRLF;
    ss << "\t\t              2h                       stop time after n hours." S_CRLF;
    ss << "\t\t              2d                       stop time after n days." S_CRLF;
    ss << "\t\t              2w                       stop time after n weeks." S_CRLF;
    ss << "\t\tstop_line:    when total line findded, then finish and quit. default value is 0, that means always running." S_CRLF;
    ss << "\t\tpath:         file path generate by 'general_table.conf' file." << S_CRLF;
    ss << "\t\tformat:       only support 'txt', default value is 'txt'" << S_CRLF;
    ss << S_CRLF;


    ss << "example: general_table.conf: " S_CRLF S_CRLF;
    ss << "\t\t #DB, DB_TEST_WRITE " S_CRLF;
    ss << "\t\t # table_lead    primary_key_field_name  table_tail_field_name   table_tail_list      file_path " S_CRLF;
    ss << "\t\t # table_lead      : prefix the table name " S_CRLF;
    ss << "\t\t # primary_key_field_name : Primary key field name can be empty " S_CRLF;
    ss << "\t\t # table_tail_field_name : Indicates the field name associated with the table name suffix Can be empty " S_CRLF;
    ss << "\t\t # table_tail_list : List of table name suffixes " S_CRLF;
    ss << "\t\t # file_path : Tabular path " S_CRLF;
    ss << "\t\t # Note: The list at the end of the table is separated by commas between each table [1, 3]. This format can be used, but the premise must be that the data and the right side is greater than the left side " S_CRLF;
    ss << "\t\t process_\t\tA, B\t./general_process.txt " S_CRLF;
    ss << "\t\t location_\t\tA, D, E, F\t./general_location.txt " S_CRLF;
    ss << "\t\t TABLE_\t\t[1, 5]\t./general_std.txt " S_CRLF;
    ss << S_CRLF;
        
    ss << "example: general_data.conf:" S_CRLF S_CRLF;
    ss << "\t\t # table_lead     : prefix the table name " S_CRLF;
    ss << "\t\t # data_path_list : List of data file paths " S_CRLF;
    ss << "\t\t # notion         : Use commas to separate the list of data file paths " S_CRLF; 
    ss << "\t\t process_\t./process-modify.csv " S_CRLF;
    ss << "\t\t location_\t./location - modify.csv " S_CRLF;
    ss << "\t\t TABLE_\t./general_std.txt.data " S_CRLF;
    ss << S_CRLF;


    ss << "example: general_std.txt: " S_CRLF S_CRLF;
    ss << "\t\t # this file for general std table " S_CRLF;
    ss << "\t\t ################################# " S_CRLF;
    ss << "\t\t FIELD_0 bool, FIELD_1 int, FIELD_2 bigint, FIELD_3 float, FIELD_4 double, FIELD_5 varchar(32), FIELD_6 timestamp " S_CRLF;
    ss << S_CRLF;


    ss << "example: general_std.txt.data: " S_CRLF S_CRLF;
    ss << "\t\t FIELD_0\tFIELD_1\tFIELD_2\tFIELD_3\tFIELD_4\tFIELD_5\tFIELD_6 " S_CRLF;
    ss << "\t\t false\t0\t0\t0.23\t0.23\t'0xxxxxxxxxxxxxxxxxxxxxxxxxxxxx32'\t'2020-01-01 00:00:00.000' " S_CRLF;
    ss << "\t\t false\t1000\t1000\t1.23\t1.23\t'1000xxxxxxxxxxxxxxxxxxxxxxxxxx32'\t'2020-01-01 00:00:01.000' " S_CRLF;
    ss << S_CRLF;
     

    std::string s = ss.str();

    // If u want to write log that size larger than 4K, must use log_write_huge, otherwise the content will be truncated to 4K.
    p->tools->log_write_huge( __FILE__, __LINE__, __FUNCTION__, LOG_INF, true, s.c_str(), s.size() );
}

static void output_welcome( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    std::stringstream ss;

    char path[ 256 ];

    ss << S_CRLF S_CRLF "Welcome to RTDB ";
#if ENABLE_TDENGINE
    ss << "& TDEngine ";
#endif // #if ENABLE_TDENGINE
    ss << "test (Powered by RTDB)" S_CRLF;

    // get current log file path
    ss << S_CRLF "log file       : " << p->tools->get_log_path() << S_CRLF;

    // get current executing program's path
    p->tools->get_exe_path( path, (int)sizeof(path) );
    ss << "exe path       : " << path << S_CRLF;

    // get current directory path
    p->tools->get_cur_dir( path, (int)sizeof(path), FALSE );
    ss << "curr dir       : " << path << S_CRLF;

    ss << "tool functions : " << (int)(sizeof(tsdb_tools_t) / sizeof(void*)) << S_CRLF;

    int conn_timeout_ms;
    int send_timeout_ms;
    int recv_timeout_ms;
    p->get_timeout( & conn_timeout_ms, & send_timeout_ms, & recv_timeout_ms );
    ss << "connect timeout: ";
    if ( (int)INFINITE == conn_timeout_ms ) {
        ss << "INFINITE";
    } else {
        ss << conn_timeout_ms;
    }
    ss << S_CRLF;
    ss << "send    timeout: ";
    if ( (int)INFINITE == send_timeout_ms ) {
        ss << "INFINITE";
    } else {
        ss << send_timeout_ms;
    }
    ss << S_CRLF;
    ss << "recv    timeout: ";
    if ( (int)INFINITE == recv_timeout_ms ) {
        ss << "INFINITE";
    } else {
        ss << recv_timeout_ms;
    }
    ss << S_CRLF;

    ss << S_CRLF;

    std::string s = ss.str();

    fputs( S_CRLF, stderr );
    TSDB_INFO( p, "%s", s.c_str() );
}

int main( int argc, char ** argv )
{
#if defined(_WIN32) ||defined(_WIN64)
    system("chcp 65001");
    WinExec( "chcp 936", SW_HIDE );
    WinExec( "mode con cols=172 lines=50", SW_HIDE );
#endif

    int r;

    // init RTDB dll/so
    if ( ! rtdb_init( NULL ) ) {
#if defined( _WIN32 ) || defined( _WIN64 )
        DWORD e = GetLastError();
        fprintf( stderr, "[GetLastError=%d]rtdb_init failed !!!!\n", (int)e );
#else
        const char * s = dlerror();
        fprintf( stderr, "[dlerror=%s]rtdb_init failed !!!!\n", s );
#endif
        return EFAULT;
    }

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    // open log file in ./LOG directory
    p->tools->log_open( "./LOG", NULL, LOG_INF );

    // enable log with screen output
    p->tools->log_enable_stderr( TRUE );

    // Show welcome message
    output_welcome( argc, argv );

    // start tick the time (ms from system start)
    unsigned long   start = p->tools->get_tick_count();

    if ( p->tools->find_argv( argc, argv, "generate.table", NULL, NULL ) ) {

        // It has -generate.table parameter.
        usage(argc, argv);
        r = EINVAL;
#if 0
        r = generate_table( argc, argv );
        if ( 0 != r ) {
            TSDB_ERROR( p, "[r=%d]generate_table failed", r );
        }
#endif

    } else if ( p->tools->find_argv( argc, argv, "create.table", NULL, NULL ) ) {

        // It has -create.table parameter.
        usage(argc, argv);
        r = EINVAL;
#if 0
        r = create_table_v2( argc, argv );
        if ( 0 != r ) {
            TSDB_ERROR( p, "[r=%d]create_table_v2 failed", r );
        }
#endif

    } else if ( p->tools->find_argv( argc, argv, "insert.table", NULL, NULL ) ) {

        // It has -insert.table parameter.
        usage(argc, argv);
        r = EINVAL;
#if 0
        r = insert_table_v2( argc, argv );
        if ( 0 != r ) {
            TSDB_ERROR( p, "[r=%d]insert_table failed", r );
        }
#endif

    } else if ( p->tools->find_argv( argc, argv, "find.table", NULL, NULL ) ) {

        // It has -find.table parameter.
        usage(argc, argv);
        r = EINVAL;
#if 0
        r = find_table_v2( argc, argv );
        if ( 0 != r ) {
            TSDB_ERROR( p, "[r=%d]find_table failed", r );
        }
#endif

    } 
    else if (p->tools->find_argv(argc, argv, "generate.data.general", NULL, NULL)) {

        // It has -generate.table parameter.

        r = generate_data_general(argc, argv);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d]generate.data.general failed", r);
        }

    }
    else if (p->tools->find_argv(argc, argv, "create.table.general", NULL, NULL)) {

        // It has -create.table parameter.

        r = create_table_general(argc, argv);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d]create_table_general failed", r);
        }

    }
    else if (p->tools->find_argv(argc, argv, "insert.table.general", NULL, NULL)) {

        // It has -insert.table parameter.

        r = insert_table_general(argc, argv);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d]insert_table_general failed", r);
        }

    }
    else if (p->tools->find_argv(argc, argv, "find.table.general", NULL, NULL)) {

        // It has -find.table parameter.

        r = find_table_general(argc, argv);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d]find_table_general failed", r);
        }

    }
    else {

        // output usage

        usage( argc, argv );
        r = EINVAL;
    }
    unsigned long   stop = p->tools->get_tick_count();
    unsigned long   span = stop - start;

    fprintf( stderr, S_CRLF );
    TSDB_INFO( p, "[use=%d ms][r=%d, %s]\n\n",
                  (int)span,
                  r,
                  strerror(r) );
    return r;
}

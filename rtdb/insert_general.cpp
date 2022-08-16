#include <sstream>
#include "wide_base.h"
#include "utils.h"
#include "file_operation.h"
#include "insert_general.h"
#include "source/source.h"
#include "source/dir/dir_source.h"
#include "source/none/none_source.h"
#include "cJSON.h"
#include "OPENTSDB/wide_opentsdb.h"
#include <assert.h>
#include <string>
#include <map>




#define INSERT_USE_TIMESTAMP    1
#define ENABLE_WRITE_INSERT_SQL 0


// 忽略opentsdb null  
#define INGORE_OPENTSDB_NULL 1


namespace rtdb
{

namespace wide
{


static inline size_t calc_start_hash( size_t table_index )
{
    return table_index * 13;
}

static bool next_bool( thread_param_insert_table_general_t * param, size_t table_i, uint64_t insert_line_index )
{
    uint64_t x = (uint64_t)calc_start_hash( table_i ) + insert_line_index;
    return 0 == ( x % 2 );
}

static int next_int( thread_param_insert_table_general_t * param, size_t table_i, uint64_t insert_line_index )
{
    uint64_t x = (uint64_t)calc_start_hash( table_i ) + insert_line_index;

    double radian = x / ( (double)60 * 2 ) * 3.1415926;
    double y      = sin( radian ) * 2 * 60;
    return (int)y;
}

static float next_float( thread_param_insert_table_general_t * param, size_t table_i, uint64_t insert_line_index )
{
    uint64_t x = (uint64_t)calc_start_hash( table_i ) + insert_line_index;

    double radian = x / ( (double)60 * 2 ) * 3.1415926;
    double y      = sin( radian ) * 2 * 60;
    return (float)y;
}

// 将sql 语句插入到数据库中 一次插入可能会设计多个表  
static int insert_into_wrapper(void* _param, const std::string& sql)
{
    int r = 0;
    thread_param_insert_table_general_t * param = (thread_param_insert_table_general_t *)_param;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    // execute the SQL return value stored to param->r.
    r = param->conn->query_non_result(sql.c_str(), sql.size());
    if (unlikely(0 != r)) {
        TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d][sql_size:%d] insert table failed",  param->thread_id, r, (int)sql.size());
        p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size());
        return r;
    }
    return r;
}


// 构建json 为 opentsdb
int build_json_for_opentsdb(
    thread_param_insert_table_general_t* param, 
    const std::string &table_name, 
    import_worker_t* worker, 
    const std::string* values, 
    std::string& sql, 
    rtdb::cJSON* root)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    rtdb::cJSON* metric = rtdb::cJSON_CreateObject();
    if (NULL == metric) {
        TSDB_ERROR(p, "[INSERT][worker_id:%d] cJSON_CreateArray is NULL not support", param->thread_id);
        return ENOMEM;
    }
    std::string db_table = param->db;
    db_table += ".";
    db_table += table_name;

    std::vector<struct test_tb_field_info_t>* vt_ttfi = worker->get_field_info();


    int values_count = 0;
    p->tools->to_const_array(values->c_str(), (int)values->length(), ",", 1, NULL, &values_count);

    std::vector<tsdb_str> vt_values;
    vt_values.resize(values_count);
    p->tools->to_const_array(values->c_str(), (int)values->length(), ",", 1, &vt_values[0], &values_count);

    sql.resize(sql.size() + 2); // {}
    // 如果是首次 少一次逗号  
    if (4 != sql.size()) {
        sql.resize(sql.size() + 1); // ,
    }
    
    // metric = db + table_name
    rtdb::cJSON_AddStringToObject(metric, "metric", db_table.c_str());
    sql.resize(sql.size() + strlen("metric") + 2);  // ""
    sql.resize(sql.size() + 1);                     // :
    sql.resize(sql.size() + strlen(db_table.c_str()) + 2); // ""
    sql.resize(sql.size() + 1);                     // ,

    // datetime 类似 '2020-01-01 00:00:00.000' 去掉前后单引号  
    uint64_t timell = p->tools->datetime_from_str(values[0].c_str() + 1, (int)(values[0].length() - 2));
    rtdb::cJSON_AddNumberToObject(metric, "timestamp", (double)timell);
    sql.resize(sql.size() + strlen("timestamp") + 2);   // ""
    sql.resize(sql.size() + 1);                         // :
    sql.resize(sql.size() + 13); // 时间戳 精确到毫秒大约都是13位数  
    sql.resize(sql.size() + 1); // ,


    rtdb::cJSON_AddNumberToObject(metric, "value", (double)timell);
    sql.resize(sql.size() + strlen("value") + 2);  // ""
    sql.resize(sql.size() + 1);                    // :
    sql.resize(sql.size() + 13);                   // 时间戳 精确到毫秒大约都是13位数  
    sql.resize(sql.size() + 1);                    // ,


    rtdb::cJSON* tags = NULL;
    tags = rtdb::cJSON_CreateObject();
    if (NULL == tags) {
        TSDB_ERROR(p, "[INSERT][worker_id:%d] cJSON_CreateObject is NULL not support", param->thread_id);
        return ENOMEM;;
    }
    sql.resize(sql.size() + 2);                      // {}
    sql.resize(sql.size() + strlen("tags") + 2);     // ""
    sql.resize(sql.size() + 1);                      // :


    // opentsdb 最多支持MAX_OPENTSDB_FILES_COUNT(9)个字段 (去除时间字段仅能支持8个)  
    for (int i = 1; i < MAX_OPENTSDB_FILES_COUNT && i < worker->get_field_count(); i++)
    {
        std::string& name = (*vt_ttfi)[i].name;
        enum tsdb_datatype_t datatype = (*vt_ttfi)[i].datatype;

        sql.resize(sql.size() + strlen(name.c_str()) + 2);      // ""
        sql.resize(sql.size() + 1);                             // :

        switch (datatype)
        {
        case TSDB_DATATYPE_BOOL:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" == value) {
#if INGORE_OPENTSDB_NULL
                // 默认为false  
                rtdb::cJSON_AddBoolToObject(tags, name.c_str(), false);
                sql.resize(sql.size() + strlen("false")); // 
#else
                rtdb::cJSON_AddNullToObject(tags, name.c_str());
                sql.resize(sql.size() + strlen("null")); // 
#endif
            }
            else {
                if ("true" == value) {
                    rtdb::cJSON_AddBoolToObject(tags, name.c_str(), true);
                }
                else {
                    rtdb::cJSON_AddBoolToObject(tags, name.c_str(), false);
                }
                sql.resize(sql.size() + strlen(value.c_str()));  // 'true' 'false'
            }
            
            break;
        }
        case TSDB_DATATYPE_INT:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" == value) {
#if INGORE_OPENTSDB_NULL
                // 默认为0  
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), 0);
                sql.resize(sql.size() + strlen("0")); // 
#else
                rtdb::cJSON_AddNullToObject(tags, name.c_str());
                sql.resize(sql.size() + strlen("null")); // 
#endif
            }
            else {
                int valuei = atoi(value.c_str());
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), valuei);
                sql.resize(sql.size() + strlen(value.c_str())); //
            }
            break;
        }
        case TSDB_DATATYPE_INT64:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" == value) {
#if INGORE_OPENTSDB_NULL
                // 默认为0  
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), 0);
                sql.resize(sql.size() + strlen("0")); // 
#else
                rtdb::cJSON_AddNullToObject(tags, name.c_str());
                sql.resize(sql.size() + strlen("null"));
#endif
            }
            else {
#if defined(_WIN32) || defined(_WIN64)
                int64_t valuell = _atoi64(value.c_str());
#else
                int64_t valuell = atoll(value.c_str());
#endif
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), (double)valuell);
                sql.resize(sql.size() + strlen(value.c_str())); // 
            }

            break;
        }
        case TSDB_DATATYPE_FLOAT:
        case TSDB_DATATYPE_DOUBLE:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" == value) {
#if INGORE_OPENTSDB_NULL
                // 默认为0  
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), 0);
                sql.resize(sql.size() + strlen("0")); // 
#else
                rtdb::cJSON_AddNullToObject(tags, name.c_str());
                sql.resize(sql.size() + strlen("null"));  // 
#endif
            }
            else {
                double valuef = atof(value.c_str());
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), valuef);
                sql.resize(sql.size() + strlen(value.c_str())); //
            }

            break;
        }

        case TSDB_DATATYPE_STRING:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" == value) {
#if INGORE_OPENTSDB_NULL
                // 默认为"null"
                rtdb::cJSON_AddStringToObject(tags, name.c_str(), "null");
                sql.resize(sql.size() + strlen("null") + 2); // ""
#else
                rtdb::cJSON_AddNullToObject(tags, name.c_str());
                sql.resize(sql.size() + strlen("null")); // 
#endif
            }
            else {
                std::string value_s = std::string(value.c_str() + 1, value.length() - 2);
                rtdb::cJSON_AddStringToObject(tags, name.c_str(), value_s.c_str());
                sql.resize(sql.size() + strlen(value.c_str())-2 + 2); // ""
            }
            break;
        }
        case TSDB_DATATYPE_DATETIME_MS:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" == value) {
#if INGORE_OPENTSDB_NULL
                // 默认为"2010-01-01 00:00:00.000" 
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), (double)1262275200000);
                sql.resize(sql.size() + 13); // 时间戳 标准毫秒 13位  
#else
                rtdb::cJSON_AddNullToObject(tags, name.c_str());
                sql.resize(sql.size() + strlen("null")); // 
#endif
            }
            else {
                uint64_t valuell = p->tools->datetime_from_str(value.c_str() + 1, (int)(value.length() - 2));
                rtdb::cJSON_AddNumberToObject(tags, name.c_str(), (double)valuell);
                sql.resize(sql.size() + 13); // 时间戳 标准毫秒 13位  

            }

            break;
        }
        case TSDB_DATATYPE_BINARY:
        case TSDB_DATATYPE_POINTER:
        case TSDB_DATATYPE_TAG_STRING:
        case TSDB_DATATYPE_UNKNOWN:
        default:
            break;
        }
        // 少记录一次  
        if (1 != i) {
            sql.resize(sql.size() + 1);  // ,
        }
    }
    
    rtdb::cJSON_AddItemToObject(metric, "tags", tags);
    rtdb::cJSON_AddItemToArray(root, metric);

    return 0;
}


// 构建json 为 influxdb
int build_json_for_influxdb(
    thread_param_insert_table_general_t* param,
    const std::string& table_name,
    import_worker_t* worker,
    const std::string* values,
    std::string& sql)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    std::stringstream ss;
 
    ss << table_name;
    ss << " ";
    

    std::vector<struct test_tb_field_info_t>* vt_ttfi = worker->get_field_info();


    int values_count = 0;
    p->tools->to_const_array(values->c_str(), (int)values->length(), ",", 1, NULL, &values_count);

    std::vector<tsdb_str> vt_values;
    vt_values.resize(values_count);
    p->tools->to_const_array(values->c_str(), (int)values->length(), ",", 1, &vt_values[0], &values_count);


    // datetime 类似 '2020-01-01 00:00:00.000' 去掉前后单引号  
    uint64_t timell = p->tools->datetime_from_str(values[0].c_str() + 1, (int)(values[0].length() - 2));
    
    // 是否是首个有效的field  
    bool is_first_real_field = false;

    for (int i = 1; i < worker->get_field_count(); i++)
    {
        std::string& name = (*vt_ttfi)[i].name;
        enum tsdb_datatype_t datatype = (*vt_ttfi)[i].datatype;

        switch (datatype)
        {
        case TSDB_DATATYPE_BOOL:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" != value) {
                if (is_first_real_field) {
                    ss << ",";
                }
                ss << name;
                ss << "=";
                if ("true" == value) {
                    ss << "true";
                }
                else {
                    ss << "false";
                }
                if (!is_first_real_field) {
                    is_first_real_field = true;
                }
            }
            break;
        }
        case TSDB_DATATYPE_INT:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" != value) {
                if (is_first_real_field) {
                    ss << ",";
                }
                ss << name;
                ss << "=";
                ss << value;
                ss << "i";
                if (!is_first_real_field) {
                    is_first_real_field = true;
                }
            }
            break;
        }
        case TSDB_DATATYPE_INT64:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" != value) {
                if (is_first_real_field) {
                    ss << ",";
                }
                ss << name;
                ss << "=";
                ss << value;
                ss << "i";
                if (!is_first_real_field) {
                    is_first_real_field = true;
                }
            }

            break;
        }
        case TSDB_DATATYPE_FLOAT:
        case TSDB_DATATYPE_DOUBLE:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" != value) {
                if (is_first_real_field) {
                    ss << ",";
                }
                ss << name;
                ss << "=";
                ss << value;
                if (!is_first_real_field) {
                    is_first_real_field = true;
                }
            }

            break;
        }

        case TSDB_DATATYPE_STRING:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" != value) {
                if (is_first_real_field) {
                    ss << ",";
                }
                ss << name;
                ss << "=";
                // 这个地方必须是双引号 单引号会报错  
                ss << "\"";
                ss << std::string(vt_values[i].ptr+1, vt_values[i].len-2);
                ss << "\"";
                //ss << value;
                if (!is_first_real_field) {
                    is_first_real_field = true;
                }
            }
            break;
        }
        case TSDB_DATATYPE_DATETIME_MS:
        {
            std::string value = std::string(vt_values[i].ptr, vt_values[i].len);
            if ("null" != value) {
                if (is_first_real_field) {
                    ss << ",";
                }
                ss << name;
                ss << "=";
                uint64_t valuell = p->tools->datetime_from_str(value.c_str() + 1, (int)(value.length() - 2));
                ss << valuell;
                ss << "000000";  // influxdb 默认时间单位为纳秒 valuell 为毫秒   
                ss << "i";
                if (!is_first_real_field) {
                    is_first_real_field = true;
                }
            }

            break;
        }
        case TSDB_DATATYPE_BINARY:
        case TSDB_DATATYPE_POINTER:
        case TSDB_DATATYPE_TAG_STRING:
        case TSDB_DATATYPE_UNKNOWN:
        default:
            break;
        }
    }

    ss << " ";
    ss << (timell*(1000) * (1000)); // influxdb 默认时间单位为纳秒 valuell 为毫秒   

    ss << "\n";

    sql += ss.str();

    return 0;
}
// 插入数据 仅仅是为了opentsdb  
int insert_into_wrapper_for_opentsdb(rtdb::cJSON* root,  std::string& sql, thread_param_insert_table_general_t* param)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    char* sql_for_opentsdb = rtdb::cJSON_PrintUnformatted(root);
    if (NULL == sql_for_opentsdb) {
        TSDB_ERROR(p, "[INSERT][worker_id:%d] cJSON_PrintUnformatted is NULL not support", param->thread_id);
        param->exited = true;
        return EINVAL;
    }

    r = param->conn->query_non_result(sql_for_opentsdb, strlen(sql_for_opentsdb));
    if (0 != r) {
        TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d][sql_size:%d] query_non_result failed",
            param->thread_id, r, (int)strlen(sql_for_opentsdb));
        return r;
    }

    if (sql_for_opentsdb) {
        rtdb::cJSON_free(sql_for_opentsdb);
        sql_for_opentsdb = NULL;
    }

    if (root) {
        rtdb::cJSON_Delete(root);
        root = NULL;
    }

    return 0;
}

/*
* 注意：这个是clickhouse插入多条语句的注意事项：  
* 例如  
* insert into table_1 (time, field) values('2020-01-01 00:00:00.000', 1);insert into table_1 (time, field) values('2020-01-01 00:00:00.000', 1);  
* 这种语句在clickhouse中不能支持的，仅插入一条，目前开发人员查看了clickhouse的源码 确认是不支持的。  
* 理由如下：  
* 1、服务器不支持多SQL语句：  
* 服务器端通过 ValuesBlockInputFormat::parseExpression 解析客户端发上来的 SQL 语句。  
* 最终调用到 ParserInsertQuery::parseImpl 解析 Insert 语句。  
* 里面遇到 ; 则被认为是查询停止符号，所以服务器是不支持多条 SQL 语句的。  
* 
* 2、客户端只有命令行工具支持多SQL语句：  
* 
* 如果命令行参数有 -multiquery 参数，则通过 ClientBase::executeMultiQuery 函数执行多条写入的逻辑。  
* 该函数调用 ClientBase::parseQuery 函数解析 SQL，最终解析SQL的代码是：ParserInsertQuery::parseImpl 函数。  
* 该函数的一部分代码如下：  
*        /// If format name is followed by ';' (end of query symbol) there is no data to insert.
*        if (data < end && *data == ';')
*            throw Exception("You have excessive ';' symbol before data for INSERT.\n"
*                                    "Example:\n\n"
*                                    "INSERT INTO t (x, y) FORMAT TabSeparated\n"
*                                    ";\tHello\n"
*                                    "2\tWorld\n"
*                                    "\n"
*                                    "Note that there is no ';' just after format name, "
*                                    "you need to put at least one whitespace symbol before the data.", ErrorCodes::SYNTAX_ERROR);
*/
void* insert_table_general_thread(void* _param)
{
    thread_param_insert_table_general_t * param = (thread_param_insert_table_general_t *)_param;

    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );


    // USE DB
    param->r = param->conn->select_db( param->db );
    if ( unlikely( 0 != param->r ) ) {
        TSDB_ERROR( p, "[r=%d][db=%s] select_db failed", param->r, param->db );
        return NULL;
    }

    // insert into each tables
    param->insert_line_count = 0;
    bool is_no_data_to_insert_table = false;

    std::vector<import_worker_t*> vt_import_worker_t;

    vt_import_worker_t.resize(param->vt_import_source_t->size());

    for (size_t i = 0; i < param->vt_import_source_t->size(); i++) {

        import_source_t* source = (*param->vt_import_source_t)[i];
        import_worker_t* worker = source->get_worker(param->thread_id, param);
        // 允许worker为空表示没有分配到任务  
        if (NULL != worker) {
            param->r = worker->open(source, param->thread_id, param);
            if (0 != param->r) {
                TSDB_ERROR(p, "[INSERT][worker_id] worker::open failed", (int)i);
                param->exited = true;
                return NULL;
            }
        }
        vt_import_worker_t[i] = worker;
    }

    int line_count = 0; 
    uint64_t insert_point = 0;
    std::string sql;
    sql.reserve(param->sql_bytes + DEFAULT_BUFFER_BYTES);
    sql.resize(0);

    if (DB_TIMESCALEDB != param->engine && DB_INFLUXDB != param->engine && DB_CLICKHOUSE != param->engine) {
        sql = "insert into ";
    }
    else {
        sql = "";
    }
   
    rtdb::cJSON* root = NULL;
    if (DB_OPENTSDB == param->engine) {
        root = rtdb::cJSON_CreateArray();
        if (NULL == root) {
            TSDB_ERROR(p, "[INSERT][worker_id:%d] cJSON_CreateArray is NULL not support", param->thread_id);
            param->exited = true;
            return NULL;
        }
        sql.resize(2); // []
    }
    
#if 0  // 这个尝试了不起作用  故废弃  
    //如果是clickhouse 允许重复插入数据  
    if (DB_CLICKHOUSE == param->engine) {
        const char* set_insert_deduplicate = "SET insert_deduplicate=0;";
        // execute the SQL return value stored to param->r.
        param->r = param->conn->query_non_result(set_insert_deduplicate, strlen(set_insert_deduplicate));
        if (unlikely(0 != param->r)) {
            TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d]SET insert_deduplicate failed", 
                param->thread_id, param->r);
            return NULL;
        }
    }
#endif

    while ( true ) {

        // 先尝试关闭  
        is_no_data_to_insert_table = true; 
        
        for (size_t i = 0; i < vt_import_worker_t.size(); i++)
        {
            import_worker_t* worker = vt_import_worker_t[i];

            const std::string* names;
            const std::string* values;
            table_names_t* tables;
            int r = 0;


            if (NULL == worker) {
                continue;
            }

            r = worker->read(names, values, tables);
            if (ENODATA == r) {
                continue;
            }

            // 如果是时间格式出错 则选择忽略此行数据  
            if (EINVALMAYEIGNORE == r) {
                is_no_data_to_insert_table = false;
                continue;
            }

            if (0 != r) {
                TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d] work::read failed",  param->thread_id, r);
                param->exited = true;
                break;
            }

            // 没有找到表名  或者 表名列表为空  则忽略  
            if (NULL == tables || tables->names.empty()) {
                continue;
            }

            is_no_data_to_insert_table = false;
            for (size_t i = 0; i != tables->names.size(); ++i) {
                if (DB_OPENTSDB == param->engine) {
                    param->r = build_json_for_opentsdb(param, tables->names[i], worker, values, sql, root);
                    if (0 != param->r) {
                        TSDB_ERROR(p, "[INSERT][worker_id:%d][table:%s] build_json_for_opentsdb failed", 
                            param->thread_id, tables->names[i].c_str());
                        param->exited = true;
                        return NULL;
                    }
                }
                else if(DB_INFLUXDB == param->engine)
                {
                    param->r = build_json_for_influxdb(param, tables->names[i], worker, values, sql);
                    if (0 != param->r) {
                        TSDB_ERROR(p, "[INSERT][worker_id:%d][table:%s] build_json_for_influxdb failed",
                            param->thread_id, tables->names[i].c_str());
                        param->exited = true;
                        return NULL;
                    }
                }
                else { // 非 OPENTSDB  
                    if (DB_TIMESCALEDB == param->engine || DB_CLICKHOUSE == param->engine) {
                        sql += "insert into ";
                    }
                    sql += tables->names[i];
                    sql += " ";
                    sql += "(";
                    sql += *names;
                    sql += " ";
                    sql += ") values (";
                    sql += *values;
                    sql += ")";
                    if (DB_TIMESCALEDB == param->engine || DB_CLICKHOUSE == param->engine) {
                        sql += ";";
                    }
                    else {
                        sql += " ";
                    }
                }
                

                line_count++;
                if (DB_OPENTSDB == param->engine) {
                    // open tsdb 最多MAX_OPENTSDB_FILES_COUNT(9)个字段  多了不支持  
                    uint64_t real_field_count = (worker->get_field_count() >= MAX_OPENTSDB_FILES_COUNT ? MAX_OPENTSDB_FILES_COUNT : worker->get_field_count());
                    insert_point += (real_field_count-1);  // 总的字段个数 - 主键  
                }
                else { // 非 opentsdb  
                    insert_point += (worker->get_field_count()-1);  // 总的字段个数 - 主键  
                }
                
                if (DB_OPENTSDB == param->engine) {
                    if (sql.size() > param->sql_bytes) {
                        param->r =  insert_into_wrapper_for_opentsdb(root, sql, param);
                        if (0 != param->r) {
                            TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d][sql_size:%d] insert_into_wrapper_for_opentsdb failed", 
                                param->thread_id, param->r, (int)sql.size());
                            param->exited = true;
                            return NULL;
                        }

                        root = rtdb::cJSON_CreateArray();
                        if (NULL == root) {
                            TSDB_ERROR(p, "[INSERT][worker_id:%d] cJSON_CreateArray is NULL not support", param->thread_id);
                            param->exited = true;
                            return NULL;
                        }
                        sql.resize(2);  // []

                        param->insert_line_count += line_count;
                        param->insert_point += insert_point;
                        line_count = 0;
                        insert_point = 0;
                    }
                    
                }
                else { // 非DB_OPENTSDB  
                    if (sql.size() > param->sql_bytes) {
                        if (DB_TIMESCALEDB != param->engine && DB_INFLUXDB != param->engine && DB_CLICKHOUSE != param->engine) {
                            sql += ";";
                        }
                        param->r = insert_into_wrapper(param, sql);
                        if (0 != param->r) {
                            TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d][sql_size:%d] insert_into_wrapper failed", 
                                param->thread_id, param->r, (int)sql.size());
                            param->exited = true;
                            return NULL;
                        }

                        if (DB_TIMESCALEDB != param->engine && DB_INFLUXDB != param->engine && DB_CLICKHOUSE != param->engine) {
                            sql = "insert into ";
                        }
                        else {
                            sql = "";
                        }

                        param->insert_line_count += line_count;
                        param->insert_point += insert_point;
                        line_count = 0;
                        insert_point = 0;
                    }
                }

                
                    
            } //  for (size_t i = 0; i != tables->names.size(); ++i) {
            
        } // for (size_t i = 0; i < vt_import_worker_t.size(); i++)

        if (is_no_data_to_insert_table) {
            break;
        }
    } // while ( true ) {

    if (line_count > 0) {
        if (!sql.empty()) {
            if (DB_TIMESCALEDB != param->engine && DB_INFLUXDB != param->engine && DB_CLICKHOUSE != param->engine) {
                sql += ";";
            }
            
            if (DB_OPENTSDB == param->engine) {
                param->r = insert_into_wrapper_for_opentsdb(root, sql, param);
                if (0 != param->r) {
                    TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d][sql_size:%d] insert_into_wrapper_for_opentsdb failed", 
                        param->thread_id, param->r, (int)sql.size());
                    param->exited = true;
                    return NULL;
                }
            }
            else { // 非 opentsdb 
                param->r = insert_into_wrapper(param, sql);
                if (0 != param->r) {
                    TSDB_ERROR(p, "[INSERT][thread_id=%d][r=%d] insert_into_wrapper failed", param->thread_id, param->r);
                    param->exited = true;
                    return NULL;
                }
            }

            
        }
        param->insert_line_count += line_count;
        param->insert_point += insert_point;
        line_count = 0;
        insert_point = 0;
    }

    // quit the thread
    param->exited = true;
    return NULL;
}

int insert_table_general( int argc, char ** argv )
{
    // get RTDB interface from TLS(thread local storage).
    // We strongly recommend that you only call this function where you need it,
    // and without storage the pointer for later use.
    tsdb_v3_t * p = rtdb_tls();
    assert( p );

    // The 'tools' structure contains many functions that we prepared for you.
    // you will see a lot of code call these functions via the 'tools'.

    int r = 0;

    TSDB_INFO( p, "[INSERT]INSERT into Table" );

    // get db type: RTDB or TDEngine
    db_type_t engine = get_db_type( argc, argv );
    if ( DB_UNKNOWN == engine ) {
        TSDB_ERROR( p, "[INSERT]get_db_type failed" );
        return r;
    }

    // -server 127.0.0.1:9000
    const char * server = NULL;
    p->tools->find_argv( argc, argv, "server", & server, NULL );
    if ( NULL == server || '\0' == * server ) {
        server = "127.0.0.1:9000";
    }
    TSDB_INFO( p, "[INSERT][PARAMETERS][server      =%s]", server );

    wide_base_t * engine2 = wide_base_t::instance( engine, argc, argv );
    if ( NULL == engine2 ) {
        TSDB_ERROR( p, "[INSERT][type=%d]create instance failed", (int)engine );
        return EFAULT;
    }

    // global connect server. RTDB supported just connect server one time.
    r = engine2->global_connect( server );
    if ( 0 != r ) {
        TSDB_ERROR( p, "[INSERT][CONNECT][server=%s]global_connect failed", server );
        return r;
    }

    // input file format.
    const char * format = NULL;
    p->tools->find_argv( argc, argv, "format", & format, NULL );
    if ( NULL == format || '\0' == * format ) {
        format = "txt";
    }
    TSDB_INFO( p, "[INSERT][PARAMETERS][format      =%s]", format );

    // input file path
    std::string path;
    {
        const char * _path = NULL;
        p->tools->find_argv( argc, argv, "path", & _path, NULL );
        if ( NULL == _path || '\0' == * _path ) {
            // 允许path为空  
            _path = "";
        }
        path = _path;
        if (!path.empty()) {
            p->tools->path_to_os( & path[0] );
        }
    }
    TSDB_INFO( p, "[INSERT][PARAMETERS][path        =%s]", path.c_str() );

    // insert table thread count
    int thread_count;
    p->tools->find_argv_int( argc, argv, "thread", & thread_count );
    if ( thread_count <= 0 ) {
        thread_count = p->tools->get_cpu_core_count();
    }
    TSDB_INFO( p, "[INSERT][PARAMETERS][thread      =%d]", thread_count );

    // insert table start time
    uint64_t start_time = 0;
    p->tools->find_argv_datetime( argc, argv, "start_time", & start_time );
    if ( 0 == start_time ) {
        start_time = p->tools->datetime_now();
    }
    char start_time_s[ 64 ];
    int  start_time_sl = (int)sizeof(start_time_s);
    p->tools->datetime_to_str( start_time, start_time_s, & start_time_sl );
    TSDB_INFO( p, "[INSERT][PARAMETERS][start_time  =%lld, %s]", start_time, start_time_s );

    // insert table stop time
    uint64_t stop_time          = 0;
    do {
        const char * s = "";
        size_t       sl= 0;
        p->tools->find_argv( argc, argv, "stop_time", & s, & sl );
        if ( NULL == s || * s <= 0 || 0 == sl ) {
            break;
        }

        stop_time = p->tools->datetime_from_str( s, (int)sl );
        if ( stop_time > 0 ) {
            break;
        }

        if ( sl < 2 || ! isdigit( * s ) ) {
            TSDB_ERROR( p, "[INSERT][PARAMETERS]invalid stop_time: %s", s );
            break;
        }

        int n = atoi( s );
        if ( n <= 0 ) {
            TSDB_ERROR( p, "[INSERT][PARAMETERS]invalid stop_time: %s", s );
        }
        //uint64_t now = p->tools->datetime_now();
        char c = s[ sl - 1 ];
        if ( 's' == c ) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)n;
        } else if ( 'm' == c ) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)n;
        } else if ( 'h' == c ) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)n;
        } else if ( 'd' == c ) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)24 * (uint64_t)n;
        } else if ( 'w' == c ) {
            stop_time = start_time + (uint64_t)1000 * (uint64_t)60 * (uint64_t)60 * (uint64_t)24 * (uint64_t)7 * (uint64_t)n;
        } else {
            TSDB_ERROR( p, "[INSERT][PARAMETERS]invalid stop_time: %s", s );
        }
    } while ( 0 );
    if ( 0 != stop_time ) {
        char stop_time_s[ 64 ];
        int  stop_time_sl = (int)sizeof(stop_time_s);
        char start_time_s[ 64 ];
        int  start_time_sl = (int)sizeof(start_time_s);
        p->tools->datetime_to_str( stop_time, stop_time_s, & stop_time_sl );
        p->tools->datetime_to_str( start_time, start_time_s, & start_time_sl );
        if ( stop_time < start_time ) {
            TSDB_ERROR( p, "[INSERT][PARAMETERS][stop_time=%lld, %s][start_time=%lld, %s]invalid time",
                           (long long)stop_time, stop_time_s, (long long)start_time, start_time_s );
            return EINVAL;
        }
        TSDB_INFO( p, "[INSERT][PARAMETERS][stop_time   =%lld, %s]", (long long)stop_time, stop_time_s );
    } else {
        TSDB_INFO( p, "[INSERT][PARAMETERS][stop_time   =NEVER]" );
    }


    // insert table stop line
    int64_t stop_line = 0;
    uint64_t stop_line_per_thread;
    uint64_t stop_line_per_thread_m;
    p->tools->find_argv_int64( argc, argv, "stop_line", & stop_line );
    if ( stop_line > 0 ) {
        stop_line_per_thread    = (uint64_t)(stop_line / (int64_t)thread_count);
        stop_line_per_thread_m  = (uint64_t)(stop_line % (int64_t)thread_count);
        TSDB_INFO( p, "[INSERT][PARAMETERS][stop_line   =%lld][stop_line_per_thread=%d]", (long long)stop_line, (long long)stop_line_per_thread );
    } else {
        TSDB_INFO( p, "[INSERT][PARAMETERS][stop_line   =NEVER]" );
        stop_line               = 0;
        stop_line_per_thread    = 0;
        stop_line_per_thread_m  = 0;
    }


    // insert table step_time
    int step_time;
    p->tools->find_argv_int( argc, argv, "step_time", & step_time );
    if ( step_time <= 0 ) {
        step_time = 1000;
    }
    TSDB_INFO( p, "[INSERT][PARAMETERS][step_time   =%d]", step_time );

    // sql size
    int sql_bytes;
    const char * ps = NULL;
    p->tools->find_argv( argc, argv, "sql_size", & ps, NULL );
    char c;
    if ( NULL == ps || '\0' == * ps || * ps <= 0 || ! isdigit( * ps ) ) {
        ps = NULL;
    } else {
        size_t len = strlen( ps );
        if ( len <= 1 ) {
            TSDB_ERROR( p, "[INSERT][PARAMETERS][sql_size=%s]invalid sql_size", ps );
            ps = NULL;
            return EINVAL;
        }
        c = ps[ len - 1 ];
    }
    if ( NULL == ps ) {
        sql_bytes = 1024 * 1024;
    } else {
        sql_bytes = atoi( ps );
        if ( 'b' == tolower(c) ) {
            // do nothing
        } else if ( 'k' == tolower(c) ) {
            sql_bytes *= 1024;
        } else if ( 'm' == tolower(c) ) {
            sql_bytes *= (1024 * 1024);
        } else {
            TSDB_ERROR( p, "[INSERT][PARAMETERS][package_size=%s]invalid package_size", ps );
            ps = NULL;
            return EINVAL;
        }
    }
    TSDB_INFO( p, "[INSERT][PARAMETERS][sql_size    =%d bytes]", sql_bytes );



    std::map < std::string, CONFIG_DATA_PATHS_SEP_T > map_vt_table_head_data_path;
    //std::vector<struct table_lead_and_table_path_t> vt_table_lead_and_table_path_t;

    // 如果路径不为空  则执行  
    if (!path.empty()) {
        // 解析表数据文件  
        r = parse_table_data_conf_file(path.c_str(), map_vt_table_head_data_path);
        if (0 != r) {
            TSDB_ERROR(p, "[CREATE][path=%s] parse_table_data_conf_file failed", path.c_str());
            return r;
        }
    }

    // input talbe config file path
    std::string table_conf;
    {
        const char* _table_conf = NULL;
        p->tools->find_argv(argc, argv, "table_conf", &_table_conf, NULL);
        if (NULL == _table_conf || '\0' == *_table_conf) {
            TSDB_ERROR(p, "[INSERT] not found table_conf");
            return EINVAL;
        }
        table_conf = _table_conf;
        p->tools->path_to_os(&table_conf[0]);
    }
    TSDB_INFO(p, "[INSERT][PARAMETERS][table_conf        =%s]", table_conf.c_str());

    // 解析表配置文件中的内容  
    std::map<std::string, struct test_table_file_info_t> map_test_table_file_info_t;
    r = parse_table_conf_file(table_conf.c_str(), map_test_table_file_info_t);
    if (0 != r) {
        TSDB_INFO(p, "[CREATE][table_conf=%s] parse_table_conf_file failed", table_conf.c_str());
        return r;
    }

#if 0
    std::vector<struct table_lead_and_table_name_t> vt_table_lead_and_table_name_t;

    // 将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
    r = convert_table_conf_map_to_table_vector(map_test_table_file_info_t, vt_table_lead_and_table_name_t);
    if (0 != r) {
        TSDB_INFO(p, "[CREATE][path=%s] convert_table_conf_map_to_table_vector failed", path.c_str());
        return r;
    }
#endif

    std::vector<import_source_t*> vt_import_source_t;
    std::vector<struct nane_source_param_t> vt_nane_source_param_t;
    std::vector<struct dir_source_param_t> vt_dir_source_param_t;
    
    
    // 表名前缀 和 路径映射关系 不为空  
    if (!map_vt_table_head_data_path.empty()) {

        vt_import_source_t.resize(map_vt_table_head_data_path.size());
        vt_dir_source_param_t.resize(map_vt_table_head_data_path.size());
        std::map < std::string, CONFIG_DATA_PATHS_SEP_T >::iterator iter = map_vt_table_head_data_path.begin();

        size_t i = 0;
        for (; iter != map_vt_table_head_data_path.end(); ++iter, i++) {

            struct dir_source_param_t dsp;
            std::string my_table_lead = iter->first;
            std::vector<std::string>& file_paths = iter->second.first;  // 路径列表  

            std::map<std::string, struct test_table_file_info_t>::iterator it = 
                map_test_table_file_info_t.find(my_table_lead);
            if (it == map_test_table_file_info_t.end()) {
                // 报错 无法找到表元信息   
                r = ENOENT;
                TSDB_ERROR(p, "[INSERT][table_lead=%s][index:%d] not found", my_table_lead.c_str(),  (int)i);
                return r;
            }
            dsp.table_lead = my_table_lead;
            dsp.vt_dirs = file_paths;
            dsp.ttfi = &it->second;
            // 分隔符号 赋值  
            dsp.sep = iter->second.second;
            vt_dir_source_param_t[i] = dsp;
            vt_import_source_t[i] = new dir_source_t();
            r = vt_import_source_t[i]->open(thread_count, &vt_dir_source_param_t[i]);
            if (0 != r) {
                TSDB_ERROR(p, "[INSERT][path=%s][index:%d] nane source open failed", path.c_str(), (int)i);
                return r;
            }
            vt_import_source_t[i]->show();
        }
    }
    else { // 临时生成数据  
        
        vt_import_source_t.resize(map_test_table_file_info_t.size());
        vt_nane_source_param_t.resize(map_test_table_file_info_t.size());
        std::map<std::string, struct test_table_file_info_t>::iterator iter =  map_test_table_file_info_t.begin();

        size_t i = 0;
        for (; iter != map_test_table_file_info_t.end(); ++iter, i++) {

            std::string my_table_lead = iter->first;
            struct nane_source_param_t nsp;
            nsp.table_lead = my_table_lead;
            nsp.ttfi = &iter->second;
            vt_nane_source_param_t[i] = nsp;
            vt_import_source_t[i] = new none_source_t();
            r = vt_import_source_t[i]->open(thread_count, &vt_nane_source_param_t[i]);
            if (0 != r) {
                TSDB_ERROR(p, "[INSERT][path=%s][index:%d] nane source open failed", path.c_str(), (int)i);
                return r;
            }
            vt_import_source_t[i]->show();
        }
    }

    std::string db_name;
    std::string sql;


    // get Database name

    {
        // first line contains Database name:
        // -db DB_TEST_WRITE
        const char* db = NULL;
        p->tools->find_argv(argc, argv, "db", &db, NULL);
        if (NULL == db || '\0' == *db) {
            db = "DB_TEST_WRITE";
        }
        TSDB_INFO(p, "[INSERT][PARAMETERS][db      =%s]", db);


        db_name = db;
        if (db_name.empty()) {
            TSDB_ERROR(p, "[INSERT]db_name empty");
            return EINVAL;
        }

        // 如果是influxdb 则顺便先创建数据 防止数据库未创建的情况  
        if (DB_INFLUXDB == engine) {
            wide_conn_t* conn = engine2->create_conn();
            if (unlikely(NULL == conn)) {
                TSDB_ERROR(p, "[CREATE][db=%s] create_conn failed", db_name.c_str());
                return EFAULT;
            }
            r = conn->select_db(db_name.c_str());
            if (unlikely(0 != r)) {
                TSDB_ERROR(p, "[CREATE][db=%s][r=%d] create database failed", db_name.c_str(), r);
                p->tools->log_write_huge(__FILE__, __LINE__, __FUNCTION__, LOG_INF, TRUE, sql.c_str(), sql.size());
                conn->kill_me();
                return r;
            }
            conn->kill_me();
            conn = NULL;
        }
       
    }

    const char* sep = NULL;
    p->tools->find_argv(argc, argv, "sep", &sep, NULL);
    if (NULL == sep || '\0' == *sep) {
        sep = DEFAULT_CSV_FILE_SEP;
    }

    std::string sep_after;

    // 分隔符号比较特殊 需要整理下  
    tidy_separate_symbol(sep, sep_after);

    TSDB_INFO(p, "[GENERATE][PARAMETERS][sep_after          =%s]", sep_after.c_str());

    // prepare INSERT into TABLE threads

    std::vector< thread_param_insert_table_general_t >   threads;
    threads.resize( thread_count );
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_insert_table_general_t & item   = threads[ i ];
        item.engine             = engine;
        item.conn           = engine2->create_conn();
        if (NULL == item.conn) {
            TSDB_ERROR(p, "[INSERT][server=%s]create_conn failed", server);
            return EFAULT;
        }
        item.thread_id          = (uint32_t)i;
        item.thread_count       = (uint32_t)threads.size();
        item.sql_bytes          = (uint32_t)sql_bytes;
        item.start_time         = start_time;
        item.stop_time          = stop_time;
        item.stop_local_time    = 0;
        item.stop_line          = stop_line_per_thread;
        if ( i == threads.size() - 1 ) {
            item.stop_line     += stop_line_per_thread_m;
        }
        item.step_time          = (uint32_t)step_time;
        item.insert_line_count  = 0;
        item.insert_need        = 0;
        item.insert_from        = 0;
        item.r                  = 0;
        item.lines              = NULL;
        item.db                 = db_name.c_str();
        item.thread             = 0;
        item.exited             = false;
        //item.map_test_table_file_info_t = &map_test_table_file_info_t;
        //item.map_vt_table_head_data_path = &map_vt_table_head_data_path;
        //item.vt_table_lead_and_table_path_t = &vt_table_lead_and_table_path_t;
        //item.vt_table_lead_and_table_name_t = &vt_table_lead_and_table_name_t;
        //item.vt_insert_table_runtime_info_t = &vt_insert_table_runtime_info_t;
        item.vt_import_source_t = &vt_import_source_t;
        item.insert_point = 0;
        item.sep = sep_after;
    }

    // start INSERT into TABLE threads

    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_insert_table_general_t & item = threads[ i ];
        item.exited = false;
        if ( unlikely( ! p->tools->thread_start( & item.thread, insert_table_general_thread, & item, 0 ) ) ) {
            TSDB_ERROR( p, "[INSERT]thread_start failed" );
            item.exited = true;
            return EFAULT;
        }
    }

    // waitting for INSERT into TABLE threads stop

    unsigned long start         = p->tools->get_tick_count();
    unsigned long last_show     = start;
    uint64_t      last_count    = 0;
    uint64_t      last_insert_point    = 0;

    while ( true ) {

        // sleep 100 ms
        p->tools->sleep_ms( 100 );

        unsigned long stop = p->tools->get_tick_count();
        unsigned long span = stop - last_show;
        if ( unlikely( span >= 1000 ) ) {

            // If last show insert info time pass 1 s, then show insert info now.

            // get current time as string, this for show to human.
            char s[ 64 ];
            int  sl = (int)sizeof(s);
            p->tools->datetime_to_str( p->tools->datetime_now(), s, & sl );

            // collect thread exit status, and insert into table count.
            uint32_t exit_count     = 0;
            uint64_t count          = 0;
            uint64_t insert_point   = 0;
            uint64_t curr_time_b    = 0;
            uint64_t curr_time_e    = 0;
            int      n;
            char     curr_time_bs[ 64 ];
            char     curr_time_es[ 64 ];
            for ( size_t i = 0; i < threads.size(); ++ i ) {
                thread_param_insert_table_general_t & item = threads[ i ];
                count += item.insert_line_count;
                insert_point += item.insert_point;
                if ( item.exited ) {
                    ++ exit_count;
                }
                if ( 0 == curr_time_b || 0 == curr_time_e ) {
                    curr_time_b = curr_time_e = item.start_time;
                } else {
                    if ( item.start_time < curr_time_b ) {
                        curr_time_b = item.start_time;
                    }
                    if ( item.start_time > curr_time_e ) {
                        curr_time_e = item.start_time;
                    }
                }
            }

            uint32_t speed = (uint32_t)(count - last_count);
            uint32_t speed_point = (uint32_t)(insert_point - last_insert_point);

            if ( curr_time_b > 0 ) {
                n = (int)sizeof(curr_time_bs);
                p->tools->datetime_to_str( curr_time_b, curr_time_bs, & n );
            } else {
                curr_time_bs[ 0 ] = '\0';
            }
            if ( curr_time_e > 0 ) {
                n = (int)sizeof(curr_time_es);
                p->tools->datetime_to_str( curr_time_e, curr_time_es, & n );
            } else {
                curr_time_es[ 0 ] = '\0';
            }

            if ( 0 == strcmp( curr_time_bs, curr_time_es ) ) {

                // [INSERT_TABLE][TOTAL ROWS COUNT:900167][SPEED=3874 (ROWS/S),5000011(POINTS/S)]
                fprintf(stderr, "\r%s [INSERT_TABLE][TOTAL ROWS COUNT:%lld][SPEED=%d (ROWS/S),%d (POINTS/S)][time=%s]...",
                        s, (long long)count, speed, speed_point, curr_time_bs);
#if 0
                fprintf( stderr, "\r%s [INSERT_TABLE][%lld / %d][speed=%d/s][time=%s]...",
                        s, (long long)count, (int)(/*lines.size()*/ 1- 1), speed,
                        curr_time_bs
                );
#endif
            } else {
                fprintf(stderr, "\r%s [INSERT_TABLE][TOTAL ROWS COUNT:%lld][SPEED=%d (ROWS/S),%d (POINTS/S)][time=%s -> %s]...",
                    s, (long long)count, speed, speed_point, curr_time_bs, curr_time_es);
#if 0
                fprintf( stderr, "\r%s [INSERT_TABLE][%lld / %d][speed=%d/s][time=%s -> %s]...",
                        s, (long long)count, (int)(/*lines.size()*/1 - 1), speed,
                        curr_time_bs, curr_time_es
                );
#endif
            }

            // member this time
            last_show = stop;
            last_count = count;
            last_insert_point = insert_point;

            if ( exit_count == (uint32_t)threads.size() ) {
                // If all thread exited, then exit the loop now.
                break;
            }
        }
    }
    fprintf( stderr, "\n" );
    
    uint64_t line_count = 0;
    uint64_t total_points = 0;
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_insert_table_general_t & item = threads[ i ];
        line_count += item.insert_line_count;
        total_points += item.insert_point;

    }

    unsigned long stop = p->tools->get_tick_count();
    unsigned long span = stop - start;
    TSDB_INFO( p, "[INSERT_TABLE][TOTAL ROWS COUNT:%lld][TOTAL POINTS:%lld][use=%d ms]",
                  (long long)line_count, (long long)total_points, span);

    // Close Thread
    for ( size_t i = 0; i < threads.size(); ++ i ) {
        thread_param_insert_table_general_t & item = threads[ i ];
        p->tools->thread_join( & item.thread );

        if ( item.conn ) {
            item.conn->kill_me();
            item.conn = NULL;
        }
    }
    print_current_path();
    return r;
}

} // namespace wide

} // namespace rtdb

#include "utils.h"
#include "dpr_stdinc.h"
#include <set>
#include <sstream>


using namespace rtdb::test::wide;






/**
 * @brief  加载文件 先按行切 注意tsdb_str 未加\n   
 * @param[in]   const char *file_path                           文件路径   
 * @param[out]  std::string &data                               文件输出到buffer中    
 * @param[out]  std::vector<tsdb_str> &vt_tsdb_str_lines      每行数据vector  
 * @return 0 成功 其他 失败    
 */
static int load_file_core(const char* file_path, std::string& data, std::vector<tsdb_str>& vt_tsdb_str_lines)
{
    int r = 0;
    int count = 0;
    int64_t data_len = 0;
    data.resize(0);

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    // 先探测文件长度  
    r = p->tools->load_file(file_path, NULL, &data_len);
    if (EMSGSIZE != r) {
         TSDB_ERROR( p, "[r=%d][file:%s] load file failed", r, file_path);
         return r;
    }

    try
    {
        data.resize((size_t)data_len);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }

    // 装载文件数据  
    r = p->tools->load_file(file_path, &data[0], &data_len);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file:%s] load file failed", r, file_path);
        return r;
    }

    if (data.empty()) {
        TSDB_ERROR(p, "[path = % s]load_file failed", file_path);
        return EINVAL;
    }

    p->tools->to_const_array(data.c_str(), (int)data.size(), "\n", 1, NULL, &count);
    if (count <= 0) {
        TSDB_ERROR(p, "[path=%s]calc count failed", file_path);
        return EINVAL;
    }

    vt_tsdb_str_lines.resize(count);
    p->tools->to_const_array(data.c_str(), (int)data.size(), "\n", 1, &vt_tsdb_str_lines[0], &count);
    if (count <= 0) {
        TSDB_ERROR(p, "[path=%s]to_const_array failed", file_path);
        return EINVAL;
    }
    vt_tsdb_str_lines.resize(count);

    return 0;
}



/**
 * @brief  将字段列表文件第一次提取 每行按,切分 同时过滤掉 空行和 #开始行   
 * @param[in]  const std::vector<tsdb_str> &vt_tsdb_str_lines           每行数据vector  
 * @param[out] std::vector<std::vector<tsdb_str> > &vt_tsdb_str_fields  每行数据按,切分后的vector  
 * @return 0 成功 其他 失败    
 */
static int do_parse_field_list_file_step_1(
    const std::vector<tsdb_str>& vt_tsdb_str_lines,
    std::vector<std::vector<tsdb_str> >& vt_tsdb_str_fields)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    try
    {
        vt_tsdb_str_fields.resize(0);
    }
    catch (const std::exception&)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
    

    for (size_t i=0; i<vt_tsdb_str_lines.size(); i++)
    {
        const tsdb_str& cs = vt_tsdb_str_lines[i];
        // 过滤掉空情况  
        if (NULL == cs.ptr || 0 == cs.len) {
            continue;
        }

        // 过滤掉起始就是‘#’号的情况  
        if (cs.len > 0 && '#' == cs.ptr[0]) {
            continue;
        }

        int count = 0;
        p->tools->to_const_array(cs.ptr, (int)cs.len, ",", 1, NULL, &count);
        

        std::vector<tsdb_str> vt_tsdb_str_field_in_one_line;

        try
        {
            vt_tsdb_str_field_in_one_line.resize(count);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        p->tools->to_const_array(cs.ptr, (int)cs.len, ",", 1, &vt_tsdb_str_field_in_one_line[0], &count);
        
        try
        {
            vt_tsdb_str_field_in_one_line.resize(count);
            vt_tsdb_str_fields.push_back(vt_tsdb_str_field_in_one_line);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }

    return 0;
}


/**
 * @brief  将字符串按指定格式分隔   
 * @param[in]  std::string& s                           原字符串  
 * @param[out] td::vector<std::string> &vt_field_unit    格式化好的字符串  
 * @return 0 成功 其他 失败    
 */
static int format_string_by_parser(std::string& str, const char* sep, std::vector<std::string> &vt_field_unit)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    parser2_t parser;
    if (!p->tools->parser_open(&parser, str.c_str(), (int)str.size())) {
        TSDB_ERROR(p, "[string:%s] parser_open failed", str.data());
        return EINVAL;
    }
    
    while (true) {
        int len;
        const char* s = p->tools->parser_read_ptr_to(&parser, sep, &len, true);
        if (NULL == s || '\0' == *s) {
            break;
        }
        vt_field_unit.push_back(std::string(s, (size_t)len));
    }


#if 0
    // 下面是自己写的 性能不高 目前有潜在bug 后续需要修改  
    int count = 0;
    std::vector<tsdb_str> vt_tsdb_str;

    p->tools->to_const_array(str.data(), (int)str.size(), sep, strlen(sep), NULL, &count);
    
    try
    {
        vt_tsdb_str.resize(count);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
    

    p->tools->to_const_array(str.data(), (int)str.size(), sep, strlen(sep), &vt_tsdb_str[0], &count);

    try
    {
        vt_field_unit.resize(count);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }


    for (int i = 0; i < count; i++) {
        try
        {
            vt_field_unit[i].assign(vt_tsdb_str[i].ptr, vt_tsdb_str[i].len);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }
    
#endif

    return 0;
}


/**
 * @brief  填充null字段 默认字段允许为空 字符串类型需要特殊处理  
 * @param[in]  std::vector<std::string>& vt_tsdb_str_one_field 一个字段信息 name type 例如  FIELD_5  varchar(32)  
 * @param[out] struct test_tb_field_info_t& ttfi  每个字段详细信息  
 * @return 0 成功 其他 失败    
 */
static int fill_null_field_except_string(std::vector<std::string>& vt_tsdb_str_one_field, struct test_tb_field_info_t& ttfi)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    int count = 0;
    
    
    try
    {
        count = (int)vt_tsdb_str_one_field.size();
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
    

    // 字段数仅仅为2个则使用默认 允许为null处理  
    if (2 == count) {
        ttfi.is_null = true;
    }
    // 可能是null  
    else if (3 == count) {
        if (0 == stricmp(vt_tsdb_str_one_field[2].data(), "null")) {
            ttfi.is_null = true;
        }
        else {
            goto _err;
        }
    }
    else if (4 == count) { // 可能是not null  
        if ( 0 == stricmp(vt_tsdb_str_one_field[2].data(), "not") && 
            0 == stricmp(vt_tsdb_str_one_field[3].data(), "null")) {
            ttfi.is_null = false;
        }
        else {
            goto _err;
        }
    }
    else { // 按无效处理  
        goto _err;
    }

    return r;

_err:
    std::string s;
    for (size_t i = 0; i < vt_tsdb_str_one_field.size(); i++) {
        if (0 != i) {
            s += " ";
        }
        s += vt_tsdb_str_one_field[i];
    }
    TSDB_ERROR(p, "invalid field null type:%s", s.data());
    return EINVAL;
}


/**
 * @brief  填充null字段 默认字段允许为空 仅仅为字符串字段服务  
 * @param[in]  std::vector<std::string>& vt_tsdb_str_one_field 一个字段信息 name type 例如  FIELD_5  varchar(32)  
 * @param[out] struct test_tb_field_info_t& ttfi  每个字段详细信息  
 * @param[out] int &end_count                     除了not null 或者null vt_tsdb_str_one_field 长度  
 * @return 0 成功 其他 失败    
 */
static int fill_null_field_for_string(
    std::vector<std::string>& vt_tsdb_str_one_field, struct test_tb_field_info_t& ttfi, int &end_count)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    int count = 0;


    try
    {
        count = (int)vt_tsdb_str_one_field.size();
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }


    // count小于2抛错  
    if (count < 2) {
        TSDB_ERROR(p, "[r=%d][count:%d] must be greater or equal to 2", EINVAL, count);
        goto _err;
    }

    // 我们乐观认为这字段是没问题的 如果有问题则在下面的函数会抛错 维护者需要注意  
    if (0 == stricmp(vt_tsdb_str_one_field[count-1].data(), "null")) {
        if (0 == stricmp(vt_tsdb_str_one_field[count-2].data(), "not")) { // 找到了 not null   
            ttfi.is_null = false;
            end_count = count - 2;
        }
        else { // 我们认为仅仅只有一个 null  
            ttfi.is_null = true;
            end_count = count - 1;
        }
    }
    else { // 没有找到 则默认为true  
        ttfi.is_null = true;
        end_count = count;
    }
   
    return r;

_err:
    std::string s;
    for (size_t i = 0; i < vt_tsdb_str_one_field.size(); i++) {
        if (0 != i) {
            s += " ";
        }
        s += vt_tsdb_str_one_field[i];
    }
    TSDB_ERROR(p, "invalid field null type:%s", s.data());
    return EINVAL;
}


/**
 * @brief  判断字段类型并填充  注意varchar 类型判断比较困难  
 * @param[in]  std::vector<std::string>& vt_tsdb_str_one_field 一个字段信息 name type 例如  FIELD_5  varchar(32)  
 * @param[out] struct test_tb_field_info_t& ttfi  每个字段详细信息  
 * @return 0 成功 其他 失败    
 */
static int judge_field_data_type_and_fill(std::vector<std::string>& vt_tsdb_str_one_field, struct test_tb_field_info_t& ttfi)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    int count = 0;


    try
    {
        count = (int)vt_tsdb_str_one_field.size();
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }


    std::string &field_type_string = vt_tsdb_str_one_field[1];
    
    if (0 == stricmp(field_type_string.data(), "timestamp")) {
        ttfi.type  = "timestamp";
        ttfi.len  = sizeof(int64_t);
        ttfi.datatype = TSDB_DATATYPE_DATETIME_MS;
        
        // 判断是否存在 null还是not null  
        r = fill_null_field_except_string(vt_tsdb_str_one_field, ttfi);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d] fill_null_field_except_string failed", r);
            return r;
        }
    }
    else if (0 == stricmp(field_type_string.data(), "bool")) {
        ttfi.type = "bool";
        ttfi.len = 1;
        ttfi.datatype = TSDB_DATATYPE_BOOL;

        // 判断是否存在 null还是not null  
        r = fill_null_field_except_string(vt_tsdb_str_one_field, ttfi);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d] fill_null_field_except_string failed", r);
            return r;
        }
    }
    else if (0 == stricmp(field_type_string.data(), "int")) {
        ttfi.type = "int";
        ttfi.len = sizeof(int);
        ttfi.datatype = TSDB_DATATYPE_INT;

        // 判断是否存在 null还是not null 
        r = fill_null_field_except_string(vt_tsdb_str_one_field, ttfi);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d] fill_null_field_except_string failed", r);
            return r;
        }
    }
    else if (0 == stricmp(field_type_string.data(), "bigint")) {
        ttfi.type = "bigint";
        ttfi.len = sizeof(int64_t);
        ttfi.datatype = TSDB_DATATYPE_INT64;

        // 判断是否存在 null还是not null 
        r = fill_null_field_except_string(vt_tsdb_str_one_field, ttfi);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d] fill_null_field_except_string failed", r);
            return r;
        }
    }
    else if (0 == stricmp(field_type_string.data(), "float")) {
        ttfi.type = "float";
        ttfi.len = sizeof(float);
        ttfi.datatype = TSDB_DATATYPE_FLOAT;

        // 判断是否存在 null还是not null 
        r = fill_null_field_except_string(vt_tsdb_str_one_field, ttfi);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d] fill_null_field_except_string failed", r);
            return r;
        }
    }
    else if (0 == stricmp(field_type_string.data(), "double")) {
        // TODO:目前 double 没有实现 替换为float   
        ttfi.type = "float";
        ttfi.len = sizeof(float);
        ttfi.datatype = TSDB_DATATYPE_FLOAT;

        // 判断是否存在 null还是not null  
        r = fill_null_field_except_string(vt_tsdb_str_one_field, ttfi);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d] fill_null_field_except_string failed", r);
            return r;
        }
    }
    else {  // 判断是否是varchar 类型  
        int field_len = (int)strlen(field_type_string.data());
        int varchar_field_len = (int)strlen("varchar");
        if (field_len >= varchar_field_len) {
            if (0 == strnicmp(field_type_string.data(), "varchar", strlen("varchar"))) {
                std::string field_type_set;
                field_type_set = field_type_string;

                int end_count = 0;
                // 探测是否是not null 还是null  
                r = fill_null_field_for_string(vt_tsdb_str_one_field, ttfi, end_count);
                if (0 != r) {
                    TSDB_ERROR(p, "[r=%d] fill_null_field_for_string failed", r);
                    return r;
                }
                // 相等   
                for (int j = 2; j < end_count; j++)
                {
                    field_type_set += vt_tsdb_str_one_field[j];
                }

                //////////////////////////////////////////////////////////////////////////////////////////////////

                count = 0;
                tsdb_str cs( field_type_set.data(), (int)field_type_set.size() );
                p->tools->to_const_array(cs.ptr, (int)cs.len, "(", 1, NULL, &count);
                if (count != 2) {
                    TSDB_ERROR(p, "[field_type=%s][count=%d] invalid count", field_type_set.data(), count);
                    return EINVAL;
                }

                std::vector<tsdb_str> vt_tsdb_str_all;
                vt_tsdb_str_all.resize(count);

                p->tools->to_const_array(cs.ptr, (int)cs.len, "(", 1, &vt_tsdb_str_all[0], &count);
                if (count != 2) {
                    TSDB_ERROR(p, "[field_type=%s][count=%d] invalid count", field_type_set.data(), count);
                    return EINVAL;
                }


                // 冗余性检查  是否是varchar 而不是其他  
                std::string varchar_string;
                varchar_string.assign(vt_tsdb_str_all[0].ptr, vt_tsdb_str_all[0].len);
                if (0 != stricmp(varchar_string.data(), "varchar")){
                    TSDB_ERROR(p, "[%s]invalid field type", varchar_string.data());
                    return EINVAL;
                }
                

                ////////////////////////////////////////////////////////////////////////////////////////


                count = 0;
                cs.ptr = vt_tsdb_str_all[1].ptr;
                cs.len = vt_tsdb_str_all[1].len;

                p->tools->to_const_array(cs.ptr, (int)cs.len, ")", 1, NULL, &count);
                if (count != 2) {
                    TSDB_ERROR(p, "[field_type=%s][count=%d] invalid count", field_type_set.data(), count);
                    return EINVAL;
                }

                std::vector<tsdb_str> vt_tsdb_str_tail;
                vt_tsdb_str_tail.resize(count);

                p->tools->to_const_array(cs.ptr, (int)cs.len, ")", 1, &vt_tsdb_str_tail[0], &count);
                if (count != 2) {
                    TSDB_ERROR(p, "[field_type=%s][count=%d] invalid count", field_type_set.data(), count);
                    return EINVAL;
                }

                // 冗余性校验  
                if ( 0 != vt_tsdb_str_tail[1].len) {
                    TSDB_ERROR(p,"[field_type=%s] invalid str", field_type_set.data());
                    return EINVAL;
                }

#if 0
                // 判断是否是数字  
                bool bret = is_all_digit(
                    vt_tsdb_str_tail[0].ptr,
                    vt_tsdb_str_tail[0].len
                );
                if (!bret) {
                    TSDB_ERROR(p, "[varchar len=%s] invalid  must all digit", field_type_set.data(), count);
                    return EINVAL;
                }

#endif
                std::string varchar_len_string;
                varchar_len_string.assign(vt_tsdb_str_tail[0].ptr, vt_tsdb_str_tail[0].len);

                int varchar_len = atoi(varchar_len_string.data());
                if (varchar_len < 1 || varchar_len >254) {
                    TSDB_ERROR(p,"[varchar=%d] invalid  must be [1, 254]", varchar_len);
                    return EINVAL;
                }

                ttfi.type = "varchar";
                ttfi.len = varchar_len;
                ttfi.datatype = TSDB_DATATYPE_STRING;
            }
            else {
                TSDB_ERROR(p,"[%s]invalid field type", field_type_string.data());
                return EINVAL;
            }
        }
        else {
            TSDB_ERROR(p,"[%s]invalid field type", field_type_string.data());
            return EINVAL;
        }
    }


    return 0;
}


/**
 * @brief  将字段列表文件第二次提取 每行按空格切分   
 * @param[in]  std::vector<std::vector<tsdb_str> >& vt_tsdb_str_fields 每个字段 vector  
 * @param[out] std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t  每个字段详细信息的vector  
 * @return 0 成功 其他 失败    
 */
static int do_parse_field_list_file_step_2(
    const std::vector<std::vector<tsdb_str> >& vt_tsdb_str_fields,
    std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t)
{
    int r = 0;
    std::set<std::string> set_field_name;
    int index = 0;


    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    try
    {
        vt_test_tb_field_info_t.resize(0);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }

    

    // 先赋值主键的信息  
    struct test_tb_field_info_t ttfi;
    ttfi.index = index++;
    ttfi.name = TSDB_COLUMN_TIME_NAME;
    ttfi.type = "timestamp";
    ttfi.len = sizeof(int64_t);
    ttfi.datatype = TSDB_DATATYPE_DATETIME_MS;
    ttfi.is_null = false; // 主键不允许为空(null)  

    try
    {
        vt_test_tb_field_info_t.push_back(ttfi);
        set_field_name.insert(TSDB_COLUMN_TIME_NAME);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
   


    for (size_t i=0; i < vt_tsdb_str_fields.size(); i++)
    {
        const std::vector<tsdb_str>& vt_tsdb_str = vt_tsdb_str_fields[i];
        for (size_t j = 0; j<vt_tsdb_str.size(); j++)
        {
            const tsdb_str& cs = vt_tsdb_str[j];

            std::string cs_string;
            try
            {
                cs_string.assign(cs.ptr, cs.len);
            }
            catch (const std::exception&)
            {
                TSDB_ERROR(p, "bad alloc");
                return ENOMEM;
            }
           
            std::vector<std::string> vt_field_unit;
            r = format_string_by_parser(cs_string, " ", vt_field_unit);
            if (0 != r) {
                TSDB_ERROR(p, "[r=%d][string:%s] format_string_by_parser failed", r, cs_string.data());
                return r;
            }

            if (vt_field_unit.size() < 2) {
                TSDB_ERROR(p, "[string:%s] illegal", cs_string.data());
                return EINVAL;
            }

            std::string &field_name = vt_field_unit[0];
           

            // 判断名字是否重复  
            std::set<std::string>::iterator iter = set_field_name.find(field_name);
            if (iter != set_field_name.end()) {
                TSDB_ERROR(p, "[%s] duplicate name", field_name.data());
                return EINVAL;
            }

            ttfi.index = index++;
            ttfi.name = field_name;
            ttfi.is_null = true;  // 除了主键外 其他字段默认允许为空(null)  

            // 判断类型 并填充  
            r = judge_field_data_type_and_fill(vt_field_unit, ttfi);
            if (0 != r) {
                std::string field_string;
                try
                {
                    field_string.assign(cs.ptr, cs.len);
                }
                catch (...)
                {
                    TSDB_ERROR(p, "bad alloc");
                    return ENOMEM;
                }
                TSDB_ERROR(p, "[r = % d][field:% s] judge_field_data_type_and_fill failed", r, field_string.data());
                return r;
            }

            try
            {
                vt_test_tb_field_info_t.push_back(ttfi);
                set_field_name.insert(ttfi.name);
            }
            catch (const std::exception&)
            {
                TSDB_ERROR(p, "bad alloc");
                return ENOMEM;
            }
            
        }
        
    }


    return 0;
}

/**
 * @brief  解析字段列表文件  
 * @param[in]  const char *field_list_file                                           文件路径  
 * @param[out] std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t     字段列表数组  
 * @return  0 成功 其他 失败  
 */
int rtdb::test::wide::parse_field_list_file(
    const char* field_list_file,
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t)
{

    int r = 0;
    std::string file_data;
    std::vector<tsdb_str> vt_tsdb_str_lines;
    std::vector<std::vector<tsdb_str> > vt_tsdb_str_fields;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    //加载文件\n 分隔  
    r = load_file_core(field_list_file, file_data, vt_tsdb_str_lines);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file:%s] load_file_core failed", r, field_list_file);
        return r;
    }


    // 将一行解析成一个一个字段单元[FIELD_0 int]  
    r = do_parse_field_list_file_step_1(vt_tsdb_str_lines, vt_tsdb_str_fields);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file=%s] do_parse_field_list_file_step_1 failed", r, field_list_file);
        return r;
    }


    // 将一个一个字段解析成我们需要格式化信息  
    r = do_parse_field_list_file_step_2(vt_tsdb_str_fields, vt_test_tb_field_info_t);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file=%s] do_parse_field_list_file_step_2 failed", r, field_list_file);
        return r;
    }

    return 0;
}







/**
 * @brief  将表列表文件第一次提取 每行按\t 切分 同时过滤掉 空行和 #开始行   
 * @param[in]  const std::vector<tsdb_str> &vt_tsdb_str_lines               每行数据vector  
 * @param[out] std::vector<std::vector<tsdb_str> > &vt_tsdb_str_table_line  每行数据按,切分后的vector  
 * @return 0 成功 其他 失败    
 */
static int do_parse_table_conf_file_step_1(
    const std::vector<tsdb_str>& vt_tsdb_str_lines,
    std::vector<std::vector<tsdb_str> >& vt_tsdb_str_table_line)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    try
    {
        vt_tsdb_str_table_line.resize(0);
    }
    catch (const std::exception&)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
    

    for (size_t i=0; i<vt_tsdb_str_lines.size(); i++)
    {
        const tsdb_str& cs = vt_tsdb_str_lines[i];
        // 过滤掉空情况  
        if (NULL == cs.ptr || 0 == cs.len) {
            continue;
        }

        // 过滤掉起始就是‘#’号的情况  
        if (cs.len > 0 && '#' == cs.ptr[0]) {
            continue;
        }

        int count = 0;
        p->tools->to_const_array(cs.ptr, (int)cs.len, "\t", 1, NULL, &count);
        

        std::vector<tsdb_str> vt_tsdb_str_table_in_one_line;

        try
        {
            vt_tsdb_str_table_in_one_line.resize(count);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        p->tools->to_const_array(cs.ptr, (int)cs.len, "\t", 1, &vt_tsdb_str_table_in_one_line[0], &count);
        
        try
        {
            vt_tsdb_str_table_in_one_line.resize(count);
            vt_tsdb_str_table_line.push_back(vt_tsdb_str_table_in_one_line);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }

    return 0;
}

/**
 * @brief  将表列表文件第二次提取 表尾部切换 相对复杂 抽离出来 单独处理     
 * @param[in]  std::vector<tsdb_str> &vt_tsdb_table_tail 后缀列表  [1, 3] 或者   a, b , c 
 * @param[in]  int count 指的是std::vector<tsdb_str> &vt_tsdb_table_tail 个数  
 * @param[out] struct test_table_file_info_t &test_table_file_info 将截取到的值放入到字段列表数组中  
 * @return 0 成功 其他 失败    
 */
static int do_parse_table_conf_file_step_2_table_tail_helper(
    std::vector<tsdb_str> &vt_tsdb_table_tail, 
    int count, 
    struct test_table_file_info_t &test_table_file_info)
{
    int r = 0;
    bool is_done = false;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    do 
    {
        // 探测是否是 [1, 3] 情况  
        if (2 == count) {
            std::string first;
            std::string last;

            first.assign(vt_tsdb_table_tail[0].ptr, vt_tsdb_table_tail[0].len);
            last.assign(vt_tsdb_table_tail[1].ptr, vt_tsdb_table_tail[1].len);


            // 去掉前后空格  
            first.erase(0, first.find_first_not_of(" "));
            first.erase(first.find_last_not_of(" ") + 1);

            // 去掉前后空格  
            last.erase(0, last.find_first_not_of(" "));
            last.erase(last.find_last_not_of(" ") + 1);

            // 探测 first 或者 last 只要有一个为空 则忽略  
            if (first.empty() || first.empty()) {
                // 什么也不做  直接返回空数组  
                test_table_file_info.vt_table_tail.resize(0);
                is_done = true;
                break;
            }

            // first 和 last 均不为空  
            if ('[' == *first.begin() && ']' == *(last.end() - 1)) {
                // 终于找到范围了  
                std::string first_real;
                std::string last_real;
                first_real = first.substr(1);
                last_real = last.substr(0, last.length() - 1);

                // 去掉前后空格  
                first_real.erase(0, first_real.find_first_not_of(" "));
                first_real.erase(first_real.find_last_not_of(" ") + 1);

                // 去掉前后空格  
                last_real.erase(0, last_real.find_first_not_of(" "));
                last_real.erase(last_real.find_last_not_of(" ") + 1);

                int first = atoi(first_real.c_str());
                int last = atoi(last_real.c_str());

                if (first > last) {
                    r = EINVAL;
                    TSDB_ERROR(p, "[r=%d][first:%d][last:%d] first > last not legal",
                        r, first, last);
                    return r;
                }
                test_table_file_info.vt_table_tail.resize(last - first + 1);
                size_t j = 0;
                for (int i = first, j = 0; i <= last; ++i, j++)
                {
                    char buf[64] = { 0 };
                    snprintf(buf, sizeof(buf), "%d", i);
                    test_table_file_info.vt_table_tail[j] = buf;
                }
                is_done = true;
                break;
            }
        }
        
        break;

    } while (0);


    if (!is_done) {
        // 表尾列表  
        for (size_t i = 0; i < vt_tsdb_table_tail.size(); i++)
        {
            try
            {
                test_table_file_info.vt_table_tail[i].assign(vt_tsdb_table_tail[i].ptr, vt_tsdb_table_tail[i].len);
                // 去掉前后空格  
                test_table_file_info.vt_table_tail[i].erase(0, test_table_file_info.vt_table_tail[i].find_first_not_of(" "));
                test_table_file_info.vt_table_tail[i].erase(test_table_file_info.vt_table_tail[i].find_last_not_of(" ") + 1);
            }
            catch (...)
            {
                TSDB_ERROR(p, "bad alloc");
                return ENOMEM;
            }
        }
    }

    return 0;
}

/**
 * @brief  将表列表文件第二次提取 每行中表尾部列表各个表之间使用逗号分割按,切分   
 * @param[in]  std::vector<std::vector<tsdb_str> >& vt_tsdb_str_fields 每个表信息 vector  
 * @param[out] std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t 表信息数组  
 *             key : 表名前缀 value : struct test_table_file_info_t  
 * @return 0 成功 其他 失败    
 */
static int do_parse_table_conf_file_step_2(
    const std::vector<std::vector<tsdb_str> >& vt_tsdb_str_table_line,
    std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t)
{
    int r = 0;
    std::set<std::string> set_field_name;
    int index = 0;


    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    map_test_table_file_info_t.clear();


    for (size_t i=0; i < vt_tsdb_str_table_line.size(); i++)
    {
        const std::vector<tsdb_str>& vt_tsdb_str = vt_tsdb_str_table_line[i];
        struct test_table_file_info_t test_table_file_info;

        test_table_file_info.index = (int)i;

        // 表名前缀  
        try
        {
            test_table_file_info.table_lead.assign(vt_tsdb_str[0].ptr, vt_tsdb_str[0].len);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        try
        {
            // 主键字段名字 可以为空  
            test_table_file_info.primary_key_field_name.assign(vt_tsdb_str[1].ptr, (int)vt_tsdb_str[1].len);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
        
        try
        {
            // 指示表名后缀关联的字段名字  
            test_table_file_info.table_tail_field_name.assign(vt_tsdb_str[2].ptr, (int)vt_tsdb_str[2].len);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        


        // 表名后缀列表  
        int count = 0;
        tsdb_str cs(vt_tsdb_str[3].ptr, (int)vt_tsdb_str[3].len);
        p->tools->to_const_array(cs.ptr, (int)cs.len, ",", 1, NULL, &count);
        // 可以允许为空  

        std::vector<tsdb_str> vt_tsdb_table_tail;
        try
        {
            vt_tsdb_table_tail.resize(count);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
        

        p->tools->to_const_array(cs.ptr, (int)cs.len, ",", 1, &vt_tsdb_table_tail[0], &count);
        // 可以允许为空  


        try
        {
            test_table_file_info.vt_table_tail.resize(count);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }


        // 探测是否是 [1, 3] 情况  
        r = do_parse_table_conf_file_step_2_table_tail_helper(
            vt_tsdb_table_tail,
            count,
            test_table_file_info);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d][file:%s] do_parse_table_conf_file_step_2_table_tail_helper",
                r, test_table_file_info.field_list_file.c_str());
            return r;
        }

        // 表结构路径  
        test_table_file_info.field_list_file.assign(vt_tsdb_str[4].ptr, (int)vt_tsdb_str[4].len);

        std::vector<struct test_tb_field_info_t> vt_test_tb_field_info_t;
        // 将此文件路径解析  
        r = rtdb::test::wide::parse_field_list_file(test_table_file_info.field_list_file.c_str(),
            test_table_file_info.vt_test_tb_field_info_t);
        if (0 != r) {
            TSDB_ERROR(p, "[r=%d][file:%s] parse_field_list_file failed", r, test_table_file_info.field_list_file.c_str());
            return r;
        }

        try
        {
            map_test_table_file_info_t.insert(std::make_pair(test_table_file_info.table_lead,test_table_file_info));
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
        
    }


    return 0;
}


/**
 * @brief  解析表配置文件  
 * @param[in]  const char *table_conf_file                                           表配置文件路径  
 * @param[out] std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t 表信息数组  
 *             key : 表名前缀 value : struct test_table_file_info_t
 * @return  0 成功 其他 失败  
 */
int rtdb::test::wide::parse_table_conf_file(
    const char* table_conf_file,
    std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t)
{
    int r = 0;


    std::string file_data;
    std::vector<tsdb_str> vt_tsdb_str_lines;

    std::vector<std::vector<tsdb_str> > vt_tsdb_str_table_line;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    //加载文件\n 分隔  
    r = load_file_core(table_conf_file, file_data, vt_tsdb_str_lines);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file:%s] load_file_core failed", r, table_conf_file);
        return r;
    }

    // 将一行解析成一个一个表格单元[process_    A, B, C D:\vs2010\test\kangminsi_process.txt]  
    r = do_parse_table_conf_file_step_1(vt_tsdb_str_lines, vt_tsdb_str_table_line);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file=%s] do_parse_table_conf_file_step_1 failed", r, table_conf_file);
        return r;
    }


    // 将一个一个表解析成我们需要格式化信息  
    r = do_parse_table_conf_file_step_2(vt_tsdb_str_table_line, map_test_table_file_info_t);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file=%s] do_parse_table_conf_file_step_2 failed", r, table_conf_file);
        return r;
    }

    return 0;
}



/**
 * @brief  将表列表文件第一次提取 每行按\t切分 同时过滤掉 空行和 #开始行   
 * @param[in]  const std::vector<tsdb_str> &vt_tsdb_str_lines               每行数据vector  
 * @param[out] std::vector<std::vector<tsdb_str> > &vt_tsdb_str_table_line  每行数据按,切分后的vector  
 * @return 0 成功 其他 失败    
 */
static int do_parse_table_data_conf_file_step_1(
    const std::vector<tsdb_str>& vt_tsdb_str_lines,
    std::vector<std::vector<tsdb_str> >& vt_tsdb_str_table_line)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    try
    {
        vt_tsdb_str_table_line.resize(0);
    }
    catch (const std::exception&)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
    

    for (size_t i=0; i<vt_tsdb_str_lines.size(); i++)
    {
        const tsdb_str& cs = vt_tsdb_str_lines[i];
        // 过滤掉空情况  
        if (NULL == cs.ptr || 0 == cs.len) {
            continue;
        }

        // 过滤掉起始就是‘#’号的情况  
        if (cs.len > 0 && '#' == cs.ptr[0]) {
            continue;
        }

        int count = 0;
        p->tools->to_const_array(cs.ptr, (int)cs.len, "\t", 1, NULL, &count);
        

        std::vector<tsdb_str> vt_tsdb_str_table_in_one_line;

        try
        {
            vt_tsdb_str_table_in_one_line.resize(count);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        p->tools->to_const_array(cs.ptr, (int)cs.len, "\t", 1, &vt_tsdb_str_table_in_one_line[0], &count);
        
        try
        {
            vt_tsdb_str_table_in_one_line.resize(count);
            vt_tsdb_str_table_line.push_back(vt_tsdb_str_table_in_one_line);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }

    return 0;
}



/**
 * @brief  将表列表文件第二次提取 每个单元是表名前缀和数据文件路径列表 注意 ： 数据文件路径列表之间使用逗号分割   
 * @param[in]  std::vector<std::vector<tsdb_str> >& vt_tsdb_str_fields 每个表信息 vector  
 * @param[out] std::map < std::string, std::vector<std::string> >& map_vt_table_head_data_path  表名前缀及文件位置映射关系  
 * @return 0 成功 其他 失败    
 */
static int do_parse_table_data_conf_file_step_2(
    const std::vector<std::vector<tsdb_str> >& vt_tsdb_str_table_line,
    std::map < std::string, std::vector<std::string> >& map_vt_table_head_data_path)
{
    int r = 0;
    std::set<std::string> set_field_name;
    int index = 0;


    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    map_vt_table_head_data_path.clear();

    for (size_t i=0; i < vt_tsdb_str_table_line.size(); i++)
    {
        const std::vector<tsdb_str>& vt_tsdb_str = vt_tsdb_str_table_line[i];
        std::string table_lead;
        std::vector<std::string> vt_table_data_path;


        // 冗余检查 vt_tsdb_str必须大于或者等于2 否则 直接报错  
        if (vt_tsdb_str.size() < (size_t)2) {
            std::string s;
            if (vt_tsdb_str.size() == 1) {
                s.assign(vt_tsdb_str[0].ptr, vt_tsdb_str[0].len);
            }
            TSDB_ERROR(p, "[index:%d][num:%d][string:%s] at least 2", (int)i, (int)vt_tsdb_str.size(), s.c_str());
            return EINVAL;
        }

        // 表名前缀  
        try
        {
            table_lead.assign(vt_tsdb_str[0].ptr, vt_tsdb_str[0].len);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        // 表数据文件列表  
        int count = 0;
        tsdb_str cs(vt_tsdb_str[1].ptr, (int)vt_tsdb_str[1].len);
        p->tools->to_const_array(cs.ptr, (int)cs.len, ",", 1, NULL, &count);
        // 可以允许为空  

        std::vector<tsdb_str> vt_tsdb_table_data;
        try
        {
            vt_tsdb_table_data.resize(count);
        }
        catch (const std::exception&)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
        

        p->tools->to_const_array(cs.ptr, (int)cs.len, ",", 1, &vt_tsdb_table_data[0], &count);
        // 可以允许为空  


        try
        {
            vt_table_data_path.resize(count);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }

        // 表尾列表  
        for (size_t i=0; i<vt_tsdb_table_data.size(); i++)
        {
            try
            {
                vt_table_data_path[i].assign(vt_tsdb_table_data[i].ptr, vt_tsdb_table_data[i].len);
                // 去掉前后空格  
                vt_table_data_path[i].erase(0, vt_table_data_path[i].find_first_not_of(" "));
                vt_table_data_path[i].erase(vt_table_data_path[i].find_last_not_of(" ") + 1);
            }
            catch (...)
            {
                TSDB_ERROR(p, "bad alloc");
                return ENOMEM;
            }
        }

        try
        {
            map_vt_table_head_data_path.insert(std::make_pair(table_lead, vt_table_data_path));
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
        
    }


    return 0;
}



/**
 * @brief  将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
 * @param[in] std::map < std::string,  std::vector<std::string> > & map_vt_table_head_data_path 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @param[in]  std::vector<struct table_lead_and_table_name_t> &vt_table_lead_and_table_name_t  表名信息vector形式  
 * @return  0 成功 其他 失败  
 */
int rtdb::test::wide::convert_table_conf_map_to_table_vector(
    const std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t,
    std::vector<struct table_lead_and_table_name_t>& vt_table_lead_and_table_name_t)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    try
    {
        vt_table_lead_and_table_name_t.resize(0);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }
    
    std::map<std::string, struct test_table_file_info_t>::const_iterator iter = map_test_table_file_info_t.begin();

    for (; iter != map_test_table_file_info_t.end(); ++iter)
    {
        const std::string &table_lead = iter->first;
        const struct test_table_file_info_t &ttfi = iter->second;

        struct rtdb::test::wide::table_lead_and_table_name_t tlatn;

        try
        {
            tlatn.table_lead = table_lead;
            for (size_t i = 0; i < ttfi.vt_table_tail.size(); i++)
            {
                // 防止尾部为空 如果为空直接忽略 因为没有表可创建   
                if (!ttfi.vt_table_tail[i].empty()) {
                    tlatn.table_name = table_lead;
                    tlatn.table_name += ttfi.vt_table_tail[i];
                    vt_table_lead_and_table_name_t.push_back(tlatn);
                }
            }
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }


    return 0;
}


/**
 * @brief  将表配置信息转化vector格式 目的是为了便于将表配置信息发送到各个线程中  
 * @param[in] const std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @param[out] std::vector<struct test_table_file_info_t*> &vt_test_table_file_info_t  表信息vector形式  
 *             注意  test_table_file_info_t * 指向输入的map 的指针 后续不用考虑释放问题  
 * @return  0 成功 其他 失败  
 */
int rtdb::test::wide::convert_table_conf_map_to_table_vector_ex(
    const std::map<std::string, struct test_table_file_info_t>& map_test_table_file_info_t,
    std::vector<struct test_table_file_info_t*>& vt_test_table_file_info_t)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    try
    {
        vt_test_table_file_info_t.resize(0);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }

    std::map<std::string, struct test_table_file_info_t>::const_iterator iter = map_test_table_file_info_t.begin();

    for (; iter != map_test_table_file_info_t.end(); ++iter)
    {
        //const struct test_table_file_info_t& ttfi = iter->second;
        try
        {
            vt_test_table_file_info_t.push_back((struct test_table_file_info_t*)&iter->second);
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }

    return 0;
}

/**
 * @brief  解析表数据文件  
 * @param[in]  const char* table_data_conf_file                                           表数据配置文件路径  
 * @param[out] std::map < std::string,  std::vector<std::string> > & map_vt_table_head_data_path 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @return  0 成功 其他 失败  
 */
int rtdb::test::wide::parse_table_data_conf_file(
    const char* table_data_conf_file,
    std::map < std::string, std::vector<std::string> >& map_vt_table_head_data_path)
{
    int r = 0;

    std::string file_data;
    std::vector<tsdb_str> vt_tsdb_str_lines;

    std::vector<std::vector<tsdb_str> > vt_tsdb_str_table_line;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    //加载文件\n 分隔  
    r = load_file_core(table_data_conf_file, file_data, vt_tsdb_str_lines);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file:%s] load_file_core failed", r, table_data_conf_file);
        return r;
    }

    // 将一行解析成一个一个表格单元[process_    D:\vs2010\test\process-modify.csv, D:\vs2010\test\process-modify.csv]  
    r = do_parse_table_data_conf_file_step_1(vt_tsdb_str_lines, vt_tsdb_str_table_line);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file=%s] do_parse_table_data_conf_file_step_1 failed", r, table_data_conf_file);
        return r;
    }


    // 将一个一个表解析成我们需要格式化信息  
    r = do_parse_table_data_conf_file_step_2(vt_tsdb_str_table_line, map_vt_table_head_data_path);
    if (0 != r) {
        TSDB_ERROR(p, "[r=%d][file=%s] do_parse_table_data_conf_file_step_2 failed", r, table_data_conf_file);
        return r;
    }

    return 0;
}


/**
 * @brief  将表数据信息转化vector格式 目的是为了便于将表数据发送到各个线程中  
 * @param[in] std::map < std::string,  std::vector<std::string> > & map_vt_table_head_data_path 表数据信息数组  
 *             key : 表名前缀  
 *             value : 数据文件列表  
 * @param[out]  std::vector<struct table_lead_and_table_path_t> &vt_table_lead_and_table_name_t  表名信息vector形式  
 * @return  0 成功 其他 失败  
 */
int rtdb::test::wide::convert_table_data_map_to_table_vector(
    const std::map < std::string, std::vector<std::string> >& map_vt_table_head_data_path,
    std::vector<struct table_lead_and_table_path_t>& vt_table_lead_and_table_path_t)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    try
    {
        vt_table_lead_and_table_path_t.resize(0);
    }
    catch (...)
    {
        TSDB_ERROR(p, "bad alloc");
        return ENOMEM;
    }

    std::map < std::string, std::vector<std::string> >::const_iterator iter = map_vt_table_head_data_path.begin();

    for (; iter != map_vt_table_head_data_path.end(); ++iter)
    {
        const std::string& table_lead = iter->first;
        const std::vector<std::string> &vt_table_data_path = iter->second;

        struct rtdb::test::wide::table_lead_and_table_path_t tlatp;

        try
        {
            tlatp.table_lead = table_lead;
            for (size_t i = 0; i < vt_table_data_path.size(); i++)
            {
                // 防止尾部为空 如果为空直接忽略 因为没有表可创建   
                if (!vt_table_data_path[i].empty()) {
                    tlatp.table_path = vt_table_data_path[i];
                    // 将路径转化为本地的文件路径  
                    p->tools->path_to_os(&tlatp.table_path[0]);
                    vt_table_lead_and_table_path_t.push_back(tlatp);
                }
            }
        }
        catch (...)
        {
            TSDB_ERROR(p, "bad alloc");
            return ENOMEM;
        }
    }

    return 0;
}


/**
 * @brief  从csv 文件中获取一行格式好的数据  
 * @param[in]   file_operation& fo                      文件操作类型 必须以读方式打开  
 * @param[in]   const char* sep                         csv 分隔符号  
 * @param[out]  std::vector<tsdb_str>& vt_data,         格式好的数据  
 * @param[out]  std::vector<BOOL>& vt_data_is_string    指示是否是字符串类型  
 * @return  0 成功 ENODATA 表示已经读到文件尾部了 其他情况失败  
 */
int rtdb::test::wide::get_format_line_from_csv_file(
    file_operation& fo,
    const char* sep,
    std::vector<tsdb_str>& vt_data,
    std::vector<BOOL>& vt_data_is_string)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    char line[8192] = { 0 };
    int data_len = fo.read_one_line(line, (unsigned int)sizeof(line));
    if (data_len <= 0) {
        if (0 == data_len) {
            // 没有数据了 正常返回   
            return ENODATA;
        }
        // 出错了  
        TSDB_ERROR(p, "read_one_line failed");
        r = EFAULT;
        return r;
    }


    data_len = (int)strlen(line);
    if (data_len >= 2 && '\r' == line[data_len - 2]) {
        line[data_len - 2] = '\0';
        data_len -= 2;
    }
    else if (data_len >= 1 && ('\r' == line[data_len - 1] || '\n' == line[data_len - 1])) {
        line[data_len - 1] = '\0';
        data_len -= 1;
    }

    int data_count = 0;

    // 先探测字段个数  
    r = p->tools->csv_line_to_array2(
        line, (int)strlen(line), sep, NULL, NULL, &data_count);
    if (0 != r) {
        TSDB_ERROR(p, "[line:%s] csv_line_to_array2 failed", line);
        return r;
    }

    vt_data.resize(data_count);
    vt_data_is_string.resize(data_count);

    // 这次真正解析出各个字段数据  
    r = p->tools->csv_line_to_array2(
        line, (int)strlen(line), sep, &vt_data[0], &vt_data_is_string[0], &data_count);
    if (0 != r) {
        TSDB_ERROR(p, "[line:%s] csv_line_to_array2 failed", line);
        return r;
    }

    return 0;
}



/**
 * @brief  为int初始化数据  
 * @param[in]  struct field_increase_store_t& fis          线程指针  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[in]  int def_value                               初始值  
 * @param[in]  float fstep                                 步长  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::init_data_for_int(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int def_value, float fstep)
{
    fis.fstep = fstep;
    fis.data.value = def_value;
    return 0;
}

/**
 * @brief  为int64初始化数据  
 * @param[in]  struct field_increase_store_t& fis          线程指针  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[in]  int64_t def_value                           初始值  
 * @param[in]  float fstep                                 步长  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::init_data_for_int64(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int64_t def_value, float fstep)
{
    fis.fstep = fstep;
    fis.data.value64 = def_value;
    return 0;
}

/**
 * @brief  为int64初始化数据  
 * @param[in]  struct field_increase_store_t& fis          线程指针  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[in]  float def_value                             初始值  
 * @param[in]  float fstep                                 步长  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::init_data_for_float(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, float def_value, float fstep)
{
    fis.fstep = ((float)fstep/(float)1000);
    fis.data.fvalue = def_value;
    return 0;
}

/**
 * @brief  为int产生数据  
 * @param[in]  struct field_increase_store_t& fis          线程指针  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] int &value                                 返回值  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::generate_data_for_int(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int& value)
{
    value = fis.data.value;
    fis.data.value += (int)fis.fstep;
    return 0;
}


/**
 * @brief  为int64产生数据  
 * @param[in]  struct field_increase_store_t& fis          线程指针  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] int64_t &value                              返回值  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::generate_data_for_int64(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, int64_t& value)
{
    value = fis.data.value64;
    fis.data.value64 += (int64_t)fis.fstep;
    return 0;
}


/**
 * @brief  为float产生数据  
 * @param[in]  struct field_increase_store_t& fis          线程指针  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] float &value                                返回值  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::generate_data_for_float(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, float& value)
{
    value = fis.data.fvalue;
    fis.data.fvalue += fis.fstep;
    return 0;
}

/**
 * @brief  为string产生数据  
 * @param[in]  struct field_increase_store_t& fis          字段增加变量结构体  
 * @param[in]  enum tsdb_datatype_t datatype               字段类型  
 * @param[out] std::string &value                          返回值  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::generate_data_for_string(struct field_increase_store_t& fis, enum tsdb_datatype_t datatype, std::string& value)
{
    int r = 0;
    int value_int = 0;

    generate_data_for_int(fis, datatype, value_int);
    std::string s;
    s.resize(64);

    snprintf(&s[0], s.length(), "%d", value_int);
    memcpy(&value[0], &s[0], strlen(s.data()) > value.length() ? value.length(): strlen(s.data()) );

    return 0;
}


/**
 * @brief  对集合初始化默认数据  
 * @param[in]  std::vector <struct field_increase_store_t> &vt_field_increase_store_t          字段增加变量结构体  
 * @param[in]  std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t               字段详细信息集合  
 * @param[in]  uint64_t start_time                         起始时间对时间戳字段有用 其他字段可忽略  
 * @param[in]  float fstep                                 步长 各个字段设置相同  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::init_data_for_batch_by_default(
    std::vector <struct field_increase_store_t>& vt_field_increase_store_t,
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t,
    uint64_t start_time,
    float fstep)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);


    for (size_t i =0; i<vt_test_tb_field_info_t.size(); ++i)
    {
        enum tsdb_datatype_t datatype = vt_test_tb_field_info_t[i].datatype;
        switch (datatype)
        {
        case TSDB_DATATYPE_BOOL:
        {
            int def_value = 0;
            init_data_for_int(vt_field_increase_store_t[i], datatype, def_value, 1.0f);
            break;
        }
        case TSDB_DATATYPE_INT:
        case TSDB_DATATYPE_STRING:
        {
            int def_value = 0;
            init_data_for_int(vt_field_increase_store_t[i], datatype, def_value, fstep);
            break;
        }
        case TSDB_DATATYPE_INT64:
        {
            int64_t def_value = 0;
            init_data_for_int64(vt_field_increase_store_t[i], datatype, def_value, fstep);
            break;
        }
            break;
        case TSDB_DATATYPE_FLOAT:
        case TSDB_DATATYPE_DOUBLE:
        {
            float def_value = 0.23f;
            // TODO : 2022-04-24 目前float 无法自动转化为科学计算法 故step 设置为1  稍微增长慢点  
            // init_data_for_float(vt_field_increase_store_t[i], datatype, def_value, step);
            init_data_for_float(vt_field_increase_store_t[i], datatype, def_value, 1.0f);
            break;
        }
        case TSDB_DATATYPE_DATETIME_MS:
        {
            int64_t def_value = start_time;
            init_data_for_int64(vt_field_increase_store_t[i], datatype, def_value, fstep);
            break;
        }
        case TSDB_DATATYPE_BINARY:
        case TSDB_DATATYPE_POINTER:
        case TSDB_DATATYPE_TAG_STRING:
        case TSDB_DATATYPE_UNKNOWN:
        default:
            TSDB_ERROR(p, "[index:%d][datatype:%d]invalid datatype", (int)i, (int)datatype);
            return EINVAL;
        }
        
    }

    return 0;
}


/**
 * @brief  根据各个字段产生一行field  
 * @param[in]  std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t               字段详细信息集合  
 * @param[in]  int start_index                                                                 从那个index 开始  
 * @param[in]  const char* sep                                                                 分隔符号  
 * @param[out] std::string &line                                                               获取的一行数据  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::generate_field_for_one_line(
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t,
    int start_index,
    const char* sep,
    std::string& line)
{
    line.resize(0);

    // 忽略 i=0 时间主键 插入字段名字  
    for (size_t i = start_index; i < vt_test_tb_field_info_t.size(); i++)
    {
        if (start_index != (int)i) {
            line += sep;
        }
        line += vt_test_tb_field_info_t[i].name;
    }

    return 0;
}


/**
 * @brief  根据各个字段产生一行数据  
 * @param[in]  std::vector <struct field_increase_store_t> &vt_field_increase_store_t          字段增加变量结构体  
 * @param[in]  std::vector<struct test_tb_field_info_t> &vt_test_tb_field_info_t               字段详细信息集合  
 * @param[in]  int start_index                                                                 从那个index 开始  
 * @param[in]  const char* sep                                                                 分隔符号  
 * @param[out] std::string &line                                                               获取的一行数据  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::generate_data_for_one_line(std::vector <struct field_increase_store_t>& vt_field_increase_store_t,
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t, int start_index, const char* sep, std::string &line);
int rtdb::test::wide::generate_data_for_one_line(std::vector <struct field_increase_store_t>& vt_field_increase_store_t,
    std::vector<struct test_tb_field_info_t>& vt_test_tb_field_info_t, int start_index,  const char* sep, std::string& line)
{
    int r = 0;

    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    line.resize(0);
 
    std::stringstream ss;

    // 忽略 i=0 时间主键 插入数据  
    for (size_t i = start_index; i < vt_test_tb_field_info_t.size(); i++)
    {
        if (start_index != (int)i) {
            ss << sep;
        }

        tsdb_datatype_t datatype = vt_test_tb_field_info_t[i].datatype;
        switch (datatype)
        {
        case TSDB_DATATYPE_BOOL:
        {
            int value = 0;
            generate_data_for_int(vt_field_increase_store_t[i], datatype, value);
            if (0 == (value % 2)) {
                ss << "false";
            }
            else
            {
                ss << "true";
            }
            break;
        }
        case TSDB_DATATYPE_INT:
        {
            int value = 0;
            generate_data_for_int(vt_field_increase_store_t[i], datatype, value);
            ss << value;
            break;
        }
        case TSDB_DATATYPE_INT64:
        {
            int64_t value = 0;
            generate_data_for_int64(vt_field_increase_store_t[i], datatype, value);
            ss << value;
            break;
        }
        case TSDB_DATATYPE_FLOAT:
        {
            float value = 0;
            char buf[128] = {0};
            generate_data_for_float(vt_field_increase_store_t[i], datatype, value);
            const char* s = p->tools->double_to_str(value, &buf[0], (int)sizeof(buf));
            if (unlikely(NULL == s || '\0' == *s)) {
                r = EINVAL;
                TSDB_ERROR(p, "[index:%d][datatype:%d] double_to_str faield", (int)i, (int)datatype);
                return r;
            }
            ss << value;
            break;
        }

        case TSDB_DATATYPE_STRING:
        {
            // snprintf Linux 和 windows 行为所有差异  
            // 为了兼容windows和linux  做出调整  
            // windows : snprintf 中的len 是不包括\0的  
            // linux : snprintf 中的len 是包括\0的  
            // snprintf 有问题 使用memcpy  
            char buf[64] = {0};
            std::string value(vt_test_tb_field_info_t[i].len, 'x');
            snprintf(buf, sizeof(buf), "%d", vt_test_tb_field_info_t[i].len);
            if (vt_test_tb_field_info_t[i].len >= 1 && vt_test_tb_field_info_t[i].len <= 9) {
                //snprintf((&value[0]+vt_test_tb_field_info_t[i].len-2), 2, "%d", vt_test_tb_field_info_t[i].len);
                memcpy((&value[0]+vt_test_tb_field_info_t[i].len-1), buf, 1);
            }
            else if (vt_test_tb_field_info_t[i].len >= 10 && vt_test_tb_field_info_t[i].len <= 99) {
                //snprintf((&value[0]+vt_test_tb_field_info_t[i].len-3), 3, "%d", vt_test_tb_field_info_t[i].len);
                memcpy((&value[0]+vt_test_tb_field_info_t[i].len-2), buf, 2);
            }
            else if (vt_test_tb_field_info_t[i].len >= 100 && vt_test_tb_field_info_t[i].len <= 999) {
                //snprintf((&value[0]+vt_test_tb_field_info_t[i].len-4), 4, "%d", vt_test_tb_field_info_t[i].len);
                memcpy((&value[0]+vt_test_tb_field_info_t[i].len-3), buf, 3);
            }
            else {
                ; // 啥也不做  
            }
            generate_data_for_string(vt_field_increase_store_t[i], datatype, value);
            ss << "\'";
            ss << value;
            ss << "\'";
            break;
        }

        case TSDB_DATATYPE_DATETIME_MS:
        {
            int64_t value = 0;
            generate_data_for_int64(vt_field_increase_store_t[i], datatype, value);
            char value_s[64];
            int  value_sl = (int)sizeof(value_s);
            p->tools->datetime_to_str(value, value_s, &value_sl);
            ss << "\'";
            ss << value_s;
            ss << "\'";
            break;
        }
        case TSDB_DATATYPE_BINARY:
        case TSDB_DATATYPE_DOUBLE:
        case TSDB_DATATYPE_POINTER:
        case TSDB_DATATYPE_TAG_STRING:
        case TSDB_DATATYPE_UNKNOWN:
        default:
            r = EINVAL;
            TSDB_ERROR(p, "[GENERATE][index:%d][datatype:%d] invalid data type", (int)i, (int)datatype);
            return r;
        }
    }

    line = ss.str();

    return 0;
}



/**
 * @brief  为字符串和时间戳增加单引号 如果之前已经加了的话 则啥也不做   
 * @param[out] std::string& value         需要将 ts 加上 并增加单引号   
 * @param[in]  tsdb_str& ts               字段详细信息集合  
 * @param[in]  enum tsdb_datatype_t datatype      数据类型 仅仅为字符串和时间戳  
 * @param[in]  datatype,   int max_len    字符串允许的最大长度  
 * @param[in]  bool is_convert_time       是否转化时间(仅仅针对时间戳类型) 目前是针对taos对时间戳兼容不好做的处理  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::add_add_single_quotes_for_string_and_timestamp(
    std::string& value, tsdb_str& ts, enum tsdb_datatype_t datatype, int max_len, bool is_convert_time)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    const char* ptr = ts.ptr;
    int             len = ts.len;

    // 数据长度为0  
    if (0 == ts.len) {
        value += "\'";
        value += "\'";
        return 0;
    }

    if ('\'' == ts.ptr[0] || '\"' == ts.ptr[0]) {
        ptr++;
    }
    

    if ('\'' == ts.ptr[ts.len - 1] || '\"' == ts.ptr[ts.len - 1]) {
        --len;
    }
   
    // 说明 头部是'或者 "
    if (ptr != ts.ptr) {
        --len;
    }
    
    std::string s;

    value += "\'";
    

    if (TSDB_DATATYPE_STRING == datatype &&  len > max_len) {
        r = EMSGSIZE;
        s.assign(ptr, max_len);
    }
    else {
        // len 有可能小于 0 或者为 负数  
        if (len > 0) {
            // 如果是时间戳 且当前为时间戳  
            if (TSDB_DATATYPE_DATETIME_MS == datatype && is_convert_time) {
                std::string timestamp_string(ptr, len);
                //int64_t time_ms = p->tools->datetime_from_str(ptr, len);
                int64_t time_ms = p->tools->datetime_from_str(timestamp_string.data(), (int)timestamp_string.length());
                char time_s[64] = {0};
                int  time_sl = (int)sizeof(time_s);
                BOOL b = p->tools->datetime_to_str(time_ms, time_s, &time_sl);
                if (!b) {
                    r = EINVAL;
                    TSDB_ERROR(p, "[r=%d][time_s:%s][time_ms:%lld] convert time failed", 
                        r, timestamp_string.data(),   (long long)time_ms);
                    return r;
                }
                s = time_s;
            }
            else {
                s.assign(ptr, len);
            }
        }
    }

    value += s;

    value += "\'";

    return r;
}


/**
 * @brief  打印当前路径  
 * @param[in]  无  
 * @return   忽略返回值  
 */
int rtdb::test::wide::print_current_path()
{

#if defined( _WIN32 ) || defined(_WIN64)
    system("cd");
#else
    system("pwd");
#endif
    return 0;
}

/**
 * @brief   判断字符串是否全部为数字   
 * @param[in]  const std::string& str     输入的字符串   
 * @param[in]  tsdb_str& ts               字段详细信息集合  
 * @param[in]  enum tsdb_datatype_t  datatype     数据类型 仅仅为字符串和时间戳  
 * @return   true 全部都是数字 false 不全是数字  
 */
bool rtdb::test::wide::is_digit_all(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    for (size_t i = 0; i < str.size(); i++)
    {
        if (!isdigit(str[i]))
        {
            return false;
        }
    }
    return true;
}


/**
 * @brief   整理bool值尽可能兼容boolean值 能够处理"true" 'true' "false" 'false' 0, 1 之类  
 * @param[out] std::string& value         处理为true 或者false   
 * @param[in]  tsdb_str& ts               字段详细信息集合  
 * @param[in]  enum tsdb_datatype_t  datatype     数据类型 仅仅为字符串和时间戳  
 * @return   0 成功 其他 错误  
 */
int rtdb::test::wide::deal_with_for_boolean(std::string& value, tsdb_str& ts, enum tsdb_datatype_t datatype)
{
    int r = 0;
    tsdb_v3_t* p = rtdb_tls();
    assert(p);

    // 仅仅是处理 bool 类型  其他类型报错  
    if (TSDB_DATATYPE_BOOL != datatype) {
        return EINVAL;
    }

    const char* ptr = ts.ptr;
    int             len = ts.len;

    // 数据长度为0  
    if (0 == ts.len) {
        value += "null";
        return 0;
    }

    if ('\'' == ts.ptr[0] || '\"' == ts.ptr[0]) {
        ptr++;
    }


    if ('\'' == ts.ptr[ts.len - 1] || '\"' == ts.ptr[ts.len - 1]) {
        --len;
    }

    // 说明 头部是'或者 "
    if (ptr != ts.ptr) {
        --len;
    }

    std::string s;
    s.assign(ptr, len);

    //判断是否是true 或者 false  
    if (0 == strnicmp(s.c_str(), "true", strlen("true"))) {
        value += "true";
        return 0;
    }

    if (0 == strnicmp(s.c_str(), "false", strlen("false"))) {
        value += "false";
        return 0;
    }
    
    //判断是否是数字  
    if (is_digit_all(s)) {
        int v = atoi(s.c_str());
        if (0 == v) {
            value += "false";
        }
        else {
            value += "true";
        }
        return 0;
    }


    return EINVAL;
}

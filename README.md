RTDB是一款高性能、高可用、跨平台、分布式、自主可控、支持SQL，具备高效的数据调度能力和分析能力的时序数据库。为满足时序场景第一需求：大数据高并发实时入库与毫秒级查询响应。不走技术路线依赖，不依赖任何开源库，重新定义底层存储架构与算法，深耕实现每行代码。

主要介绍怎么使用RTDB完成数据的高效导入与查询。同时为了让大家对RTDB更深入客观的了解，该项目也适配了TDEngine与Timescale的数据导入与查询操作。
请分别参阅
TDEngine ：https://github.com/taosdata/TDengine
Timescale: https://github.com/timescale/timescaledb


# Features
- **支持的数据类型**
下表所示RTDB支持的数据类型。其中，“**数据类型及别名**”列中所示的内容都可以在SQL语句中使用。我们在SQL99标准以外，支持更多的数据类型别名，是为了与更广泛的与其它数据库兼容。
   
   支持的数据类型如下表：
   
  | 数据类型及别名                    | 描述                  | 格式 |
  | ---------------------          | ----                  | --  |
  | timestamp, datetime            | 时间戳                 | ‘2022-03-01’
  |                                |                       | '2022-03-01 00:00:00'
  |                                |                       | '2022-03-01 00:00:00.000'
  |                                |                       | 1646064000000
  | boolean, bool                  | 布尔类型                | true
  |                                |                       | false, 
  |                                |                       | yes
  |                                |                       | no
  | int,     int32, int4, smallint, tinyint, int2, int16 | 32位整数。[注1] | -2147483648
  |                                |                       | 2147483646
  | bigint,  int64, int8           | 64位整数 | -4611686018427387902
  |                                |                       | 4611686018427387902
  | float,   real, single          | 单精度浮点              | 1.0
  | double,  double precision      | 双精度浮点              | 1.0
  | varchar(?), string(?), char(?) | 字符串类型 [注2]        | 'string'
 
  [注1] 目前 smallint, tinyint, int2, int16 这种1字节，或者2字节的整型，在数据库中使用4字节整型int存储。
  [注2] 在测试版本中，varchar 采用定长字符串 char(?) 技术。
  
- **支持表结构模板预先申明，根据表结构导入数据**
该项目是根据表结构的模板创建表，模板中定义了表的字段名称、字段类型以及字段是否支持空。系统保留字段名称“time”作为主键，如果表结构中没有申明“time”字段，系统根据记录记录导入时间自动生成时间戳作为主键。
表结构模板格式如下：

```shell
# this file for location.csv table
############################################################################################################################

# 发动机唯一标识 uuid 64 A
gps_vin varchar(64) not null
# gps接收时间 B 
gps_receive_time timestamp
#  全球定位系统 D
gps_acc bool
# gps 卫星数 F
gps_satellites_num int 
# 全球定位系统纬度 G 
gps_latitude float
# 速度 I 
gps_vehicle_speed float
```
- **支持文本数据与csv数据的导入**
不论是文本结构还是csv结构，文件的第一行是字段名称，之后才能是数据。字段名称必须要与表结构模板申明中的字段名保持一致。如此系统会根据数据文件中的字段名称匹配上表结构关于字段类型的描述。列与列之间的分隔符可以是：空格“ ”、逗号“，”、顿号“、”。

- **支持模拟数据的导入**
如果在配置中没有指定需要导入的数据文件，系统会智能匹配表结构模板中的数据类型，随机生成数据，执行数据导入。

## Requirements
主程序，在 Windows 下的可执行文件名是：rtdb_import.exe，在 Linux 下的可执行文件名是：rtdb_import

在程序同目录下，包含数据库连接客户端的动态连接库文件。

  |             | 操作系统      | 依赖文件                |        描述                       |
  | ----        |  -           | --                    |  ---                             |
  |  RTDB       | Windows32位  | tsdb.dll              | RTDB数据库的32位Windows客户端接口库 |
  |             | Windows64位  | tsdb.dll              | RTDB数据库的64位Windows客户端接口库 |
  |             | Linux 64位   | libtsdb.so            | RTDB数据库的64位Linux客户端接口库   |
  |  TDEngine   | Windows32位  | taos.dll              | TDEngine数据库的32位Windows客户端接口库 |
  |             | Windows64位  | taos.dll              | TDEngine数据库的64位Windows客户端接口库 |
  |             | Linux 64位   | libtsdb.so            | TDEngine数据库的64位Linux客户端接口库   |
  |  TimeScale  | Windows64位  | libcrypto-1_1-x64.dll |                                     |
  |             |              | libiconv-2.dll        |                                     |
  |             |              | libintl-9.dll         |                                     |
  |             |              | libpq.dll             |                                     |
  |             |              | libpq.lib             |                                     |
  |             |              | libssl-1_1-x64.dll    |                                     |
  |             | Linux 64位   | libpq.so              |                                     |


RTDB 数据库客户端接口是动态装载的，所以对运行环境没有任何要求。
TAOS 数据库客户端接口只能通过 -ltaos 编译选项连接，所以要求 LD_LIBRARY_PATH 环境变量中必须包括 libtaos.so.1 文件
由于timescale是基于postgres的，所以如果要编译适配timescale，需要引入postgres的依赖。

**编译器配置**
windows，vs2010
linux，gcc


## Build
- **windows**
执行wide_table.sln编译

- **linux**
cd \wide_table\rtdb
执行 make

## Installation

数据库服务安装

RTDB数据库服务安装请参考：（https://rtdb.com/document?id=62273cc6aa303062f31a644a）

TDengine数据库服务安装请参考：(https://github.com/taosdata/TDengine)

Timescale数据库服务安装请参考：(https://github.com/timescale/timescaledb)

## Configuration
```shell

generate test data:

        rtdb_import -generate.data.general  -thread 2 -path ./general_table.conf -format txt  -start_time 2020-01-01 -step_time 1000 -stop_time 2020-02-01 -line_count 100
                thread:       how many threads used to create data, default thread count same with CPU core count
                path:         file path generate by 'general_table.conf' file.
                format:       only support 'txt', default value is 'txt'
                start_time:   start time.
                step_time:    after writing data to all tables once, the timestamp adjusted to increment a fixed interval. This value is the fixed interval.
                stop_time:    stop time. 
                              YYYY-MM-DD               stop date string.
                              YYYY-MM-DD HH:mm:SS.000  stop time string.
                              2s                       stop time after n seconds.
                              2m                       stop time after n minutes.
                              2h                       stop time after n hours.
                              2d                       stop time after n days.
                              2w                       stop time after n weeks.
                line_count:   need create lines one tables. 
                              line_count and stop_time cannot be 0 at the same time..
                              line_count priority is greater than stop_time .

create table:

        rtdb_import -create.table.general -engine rtdb -server 127.0.0.1:9000 -thread 3 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path ./general_table.conf -format txt  -db DB_TEST_WRITE
        rtdb_import -create.table.general -engine taos -server 192.168.1.43:6030 -thread 3 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path ./general_table.conf -format txt  -db DB_TEST_WRITE
        rtdb_import -create.table.general -engine timescaledb -server 192.168.1.43:5432 -thread 3 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path ./general_table.conf -format txt  -db DB_TEST_WRITE
                engine:       rtdb      RTDB (http://www.rtdb.com).
                              taos      TAOS (http://www.taosdata.com).
                              timescaledb      TIMESCALEDB (https://www.timescale.com/).
                server:       server address, format is 'ip:port'.
                thread:       how many threads used to create table, default thread count same with CPU core count
                timeout.conn: socket connect timeout by ms. '3000' by default.
                timeout.send: socket connect timeout by ms. 'infinite' by default.
                timeout.recv: socket connect timeout by ms. '3000' by default.
                path:         file path generate by 'general_table.conf' file.
                format:       only support 'txt', default value is 'txt'
                db:           which database will be createdd. default value is 'DB_TEST_WRITE'

insert into table:

        rtdb_import -insert.table.general -engine rtdb -server 127.0.0.1:9000 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000 -stop_time 1m  -sql_size 128k -db DB_TEST_WRITE -table_conf ./general_table.conf
        rtdb_import -insert.table.general -engine taos -server 192.168.1.43:6030 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000 -stop_time 1m -sql_size 128k -db DB_TEST_WRITE -table_conf ./general_table.conf
        rtdb_import -insert.table.general -engine timescaledb -server 192.168.1.43:5432 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000 -stop_time 1m -sql_size 128k -db DB_TEST_WRITE -table_conf ./general_table.conf
                engine:       rtdb      RTDB (http://www.rtdb.com).
                              taos      TAOS (http://www.taosdata.com).
                              timescaledb      TIMESCALEDB (https://www.timescale.com/).
                server:       server address, format is 'ip:port'.
                thread:       how many threads used to write data, default thread count same with CPU core count
                timeout.conn: socket connect timeout by ms. '3000' by default.
                timeout.send: socket connect timeout by ms. 'infinite' by default.
                timeout.recv: socket connect timeout by ms. '3000' by default.
                start_time:   start time.
                step_time:    after writing data to all tables once, the timestamp adjusted to increment a fixed interval. This value is the fixed interval.
                              unit is ms, default value is 1000, that means 1 second.
                stop_time:    stop time. path exists Automatically lapse
                              YYYY-MM-DD               stop date string.
                              YYYY-MM-DD HH:mm:SS.000  stop time string.
                              2s                       stop time after n seconds.
                              2m                       stop time after n minutes.
                              2h                       stop time after n hours.
                              2d                       stop time after n days.
                              2w                       stop time after n weeks.
                sql_size:     max bytes of an SQL statement, number followed by a charactor Suggest 128K
                path:         file path generate by 'general_data.conf' file. if path not exist The mode is to generate data automatically!!!
                format:       only support 'txt', default value is 'txt'
                db:           which database will be createdd. default value is 'DB_TEST_WRITE'
                table_conf:   file path generate by 'general_table.conf' file.

find from table:

        rtdb_import -insert.table.general -engine rtdb -server 127.0.0.1:9000 -thread 80 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 1m -stop_line 1000 -db DB_TEST_WRITE -path ./general_table.conf -format txt
        rtdb_import -find.table.general -engine taos -server 127.0.0.1:6030 -thread 80 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 1m -stop_line 1000 -db DB_TEST_WRITE -path ./general_table.conf -format txt
        rtdb_import -find.table.general -engine timescaledb -server 127.0.0.1:5432 -thread 80 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 1m -stop_line 1000 -db DB_TEST_WRITE -path ./general_table.conf -format txt
                engine:       rtdb      RTDB (http://www.rtdb.com).
                              taos      TAOS (http://www.taosdata.com).
                              timescaledb      TIMESCALEDB (https://www.timescale.com/).
                server:       server address, format is 'ip:port'.
                thread:       how many threads used to find data, default thread count same with CPU core count
                timeout.recv: socket connect timeout by ms. '3000' by default.
                start_time:   start time.
                step_time:    after find data for all tables once, the timestamp adjusted to increment a fixed interval. This value is the fixed interval.
                              unit is ms, default value is 3600000, that means 1 hour.
                stop_time:    stop time.  default forever
                              YYYY-MM-DD               stop date string.
                              YYYY-MM-DD HH:mm:SS.000  stop time string.
                              2s                       stop time after n seconds.
                              2m                       stop time after n minutes.
                              2h                       stop time after n hours.
                              2d                       stop time after n days.
                              2w                       stop time after n weeks.
                stop_line:    when total line findded, then finish and quit. default value is 0, that means always running.
                path:         file path generate by 'general_table.conf' file.
                format:       only support 'txt', default value is 'txt'

example: general_table.conf: 

                 #DB, DB_TEST_WRITE 
                 # table_lead    primary_key_field_name  table_tail_field_name   table_tail_list      file_path 
                 # table_lead      : prefix the table name 
                 # primary_key_field_name : Primary key field name can be empty 
                 # table_tail_field_name : Indicates the field name associated with the table name suffix Can be empty 
                 # table_tail_list : List of table name suffixes 
                 # file_path : Tabular path 
                 # Note: The list at the end of the table is separated by commas between each table [1, 3]. This format can be used, but the premise must be that the data and the right side is greater than the left side 
                 process_               A, B    ./general_process.txt 
                 location_              A, D, E, F      ./general_location.txt 
                 TABLE_         [1, 5]  ./general_std.txt 

example: general_data.conf:

                 # table_lead     : prefix the table name 
                 # data_path_list : List of data file paths 
                 # notion         : Use commas to separate the list of data file paths 
                 process_       ./process-modify.csv 
                 location_      ./location - modify.csv 
                 TABLE_ ./general_std.txt.data 

example: general_std.txt: 

                 # this file for general std table 
                 ################################# 
                 FIELD_0 bool, FIELD_1 int, FIELD_2 bigint, FIELD_3 float, FIELD_4 double, FIELD_5 varchar(32), FIELD_6 timestamp 

example: general_std.txt.data: 

                 FIELD_0        FIELD_1 FIELD_2 FIELD_3 FIELD_4 FIELD_5 FIELD_6 
                 false  0       0       0.23    0.23    '0xxxxxxxxxxxxxxxxxxxxxxxxxxxxx32'      '2020-01-01 00:00:00.000' 
                 false  1000    1000    1.23    1.23    '1000xxxxxxxxxxxxxxxxxxxxxxxxxx32'      '2020-01-01 00:00:01.000' 
```

## Usage
1、用如下命令生成 60 万张表的信息。

> rtdb_import.exe -generate.table -dst ./generate_table.txt -format txt -db DB_wide -table_lead TABLE_ -bool 200000 -int 200000 -float 200000
{.is-info}









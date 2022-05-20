<a href="https://www.rtdb.com" target = "_blank">
 <img border="0" src="https://rtdb-doc.oss-cn-beijing.aliyuncs.com/quick_learn/log_rtdb.png?versionId=CAEQHhiBgIDUvLPvhhgiIGFjMWIyNTU2N2RhNTRiODk5N2FmMjNlZDNiZmYzYTMz" height="100"/></a>


RTDB是一款高性能、高可用、跨平台、分布式、自主可控、支持SQL，具备高效的数据调度能力和分析能力的时序数据库。为满足时序场景第一需求：大数据高并发实时入库与毫秒级查询响应，重新定义、设计数据库底层文件存储结构和上层数据结构组织，采用：纯列式存储、行索引、宽表数据结构，具备以下特点：
* **1. 高性能**
在2颗物理CPU,128G内存，机械硬盘的配置下，标签点规模达到500万点的情况下，单机写入速度达到每秒500万点以上，且系统的内存占用严格控制在20%以下。即使在如此高并发的情况下，依然能做到毫秒级查询响应时间。实时海量数据无损压缩率达到6%；

* **2. 低成本**
RTDB出色的单机性能表现，降低或避免了业务服务的水平扩展需求，不仅大幅降低存储空间与服务器资源投入，也大幅降低了运维投入；

* **3. 一栈式**
RTDB一个产品就能满足时序业务对数据存储、缓存、计算的需求，无需集成Redis、HBase、Spark等系统，将业务实现变得干净而纯粹；

* **4. 支持失序写入**
虽然时序数据大部分是有序的，但是数据在生产与传递过程中可能会由于硬件抖动、网络抖动导致一定区间范围内的失序。RTDB采用独创的分层轮盘接收窗口排序合并的算法，在保障不影响系统稳定和性能的前提下实现失序数据的有序写入，业务方无需担心时序数据失序的问题；

* **5. 简单易用**
RTDB支持标准的SQL以及适配主流的时序数据库的操作规范，尽可能的把逻辑操作简化，减少学习成本和切换成本；

* **6. 自主可控**
从0开始构建的RTDB，不依赖任何第三方库；

* **7. 原生跨平台**
一套代码无缝原生跨Linux、windows等多种操作系统；

使用RTDB_IMPORT做为数据导入工具既可以往RTDB时序数据存储引擎完成时序数据的高效导入与查询，同时为了让大家对RTDB有更深入客观的了解，该项目也适配了TDEngine与Timescale的数据导入与查询操作。

关于TDEngine与Timescale请分别参阅：

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
  |             |              | libwinpthread-1.dll   |                                     |
  |             | Linux 64位   | libpq.so              |                                     |


RTDB 数据库客户端接口是动态装载的，所以对运行环境没有任何要求。
TAOS 数据库客户端接口只能通过 -ltaos 编译选项连接，所以要求 LD_LIBRARY_PATH 环境变量中必须包括 libtaos.so.1 文件
由于timescale是基于postgres的，所以如果要编译适配timescale，需要引入postgres的依赖。

**编译器配置**

> windows vs2010

> linux，gcc

## Build
- **windows**

> 执行rtdb_import.sln完成编译

- **linux**

> cd make

> 执行 sh build.sh

## Installation

数据库服务安装

RTDB数据库服务安装请参考：（https://rtdb.com/document?id=62273cc6aa303062f31a644a）

TDengine数据库服务安装请参考：(https://github.com/taosdata/TDengine)

Timescale数据库服务安装请参考：(https://github.com/timescale/timescaledb)

## Usage
### 1、创建表
通过以下命令创建5千张表
#### windows平台（create_table_general.bat）
> call "rtdb_import.exe" -create.table.general -engine rtdb -server 192.168.1.43:9000 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path data\general_table.conf -format txt  -db DB_TEST_WRITE
{.is-info}

#### linux平台（create_table_general.sh）
> ./rtdb_import -create.table.general -engine rtdb -server 192.168.1.43:9000 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path data/general_table.conf -format txt  -db DB_TEST_WRITE
{.is-info}

### 创建表参数解释

  |   参数名称              | 参数说明            |        描述                           |
  | ----                  |  -                 |   ---                                |
  | create.table.general  | 执行建表操作         |                                       |
  | engine                | 执行哪种数据库引擎    | 目前分别支持: rtdb,taos,timescaledb     |
  | server                | 数据库服务的host url | 服务器ip地址与端口号                     |
  | thread                | 线程数              |                                      |
  | timeout.conn          | 网络连接超时时间      |  单位毫秒，  infinite：表示没有超时时间   |
  | timeout.send          | 发送网络消息超时时间   |  单位毫秒，  infinite：表示没有超时时间   |
  | timeout.recv          | 接收网络消息超时时间   |  单位毫秒，  infinite：表示没有超时时间   |
  | path                  | 创建表的结构模板文件   |                                      |
  | format                | 模板文件的格式        | 目前只支持txt                          |
  | db                    | 数据库名称           |                                      |
   
### general_table.conf 格式说明

> TABLE_			    [1, 5000] 	data/general_std.txt
{.is-info}

  |   参数                 | 参数说明                                                     | 
  | ----                  |  -                                                          |  
  | TABLE_                | 表名称的前缀，由用户自定义，如：process_、location_               | 
  | [1, 5000]             | 表名称的后缀，也可以理解表名的下标范围                             |  
  |                       | 系统根据表名的前缀与后缀范围，构建表名称                           | 
  |                       | 如：TABLE_1,TABLE_2，....                                    | 
  | data/general_std.txt  | 表格式模板，用户通过模板定义表的字段、类型、大小以及是否支持空         | 
  
### 2、导入数据
通过以下命令导入数据。
*注：如果命令中不指定path参数，也就是不指定数据源，系统会根据表结构中字段的数据类型，模拟生成数据。*
#### windows平台（import_data.bat）
> call "rtdb_import.exe" -insert.table.general -engine rtdb -server 192.168.1.43:1234 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  /start_time '2020-01-01' -step_time 1000  -sql_size 128k -path data\general_data.conf -format csv -db DB_TEST_WRITE -table_conf data\general_table.conf
{.is-info}

#### linux平台（import_data.sh）
> ./rtdb_import -insert.table.general -engine rtdb -server 192.168.1.43:1234 -thread 40 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000  -stop_time 1h -sql_size 128k -path data/general_data.conf -format csv  -db DB_TEST_WRITE -table_conf data/general_table.conf
{.is-info}

### 导入数据参数解释

  |   参数名称              | 参数说明            |        描述                           |
  | ----                  |  -                 |   ---                                |
  | insert.table.general  | 执行插入数据操作         |                                       |
  | engine                | 执行哪种数据库引擎    | 目前分别支持: rtdb,taos,timescaledb     |
  | server                | 数据库服务的host url | 服务器ip地址与端口号                     |
  | thread                | 线程数              |                                      |
  | timeout.conn          | 网络连接超时时间      |  单位毫秒，  infinite：表示没有超时时间   |
  | timeout.send          | 发送网络消息超时时间   |  单位毫秒，  infinite：表示没有超时时间   |
  | timeout.recv          | 接收网络消息超时时间      |  单位毫秒，  infinite：表示没有超时时间   |
  | start_time            | 导入数据的开始时间        |                                     |
  | step_time             | 相邻两条数据之间的时间间隔 | 单位毫秒                               |
  | sql_size              | 最大sql命令的字节数   | 建议128K                               |
  | path                  | 导入数据的配置信息文件   |                                      |
  | format                | 导入数据文件的格式        | 目前支持txt与csv                          |
  | db                    | 数据库名称           |                                      |
  | table_conf            | 表的结构模板文件      |                                      |
   
### general_data.conf  导入数据的配置说明

> TABLE_	data/general_std.txt.data
{.is-info}

  |   参数                 | 参数说明                                                     | 
  | ----                  |  -                                                          |  
  | TABLE_                | 该前缀对应的表的数据源文件                                       | 
  
  
### general_std.txt.data  导入数据源格式说明
不论是文本结构还是csv结构，文件的第一行是字段名称，之后才能是数据。字段名称必须要与表结构模板申明中的字段名保持一致。如此系统会根据数据文件中的字段名称匹配上表结构关于字段类型的描述。列与列之间的分隔符可以是：空格“ ”、逗号“，”、顿号“、”。

> FIELD_0	FIELD_1	FIELD_2	FIELD_3	FIELD_4	FIELD_5	FIELD_6	FIELD_7	FIELD_8	FIELD_9	FIELD_10	FIELD_11	FIELD_12	FIELD_13
> false	0	0	0.23	0.23	'0xxxxxxxxxxxxxxxxxxxxxxxxxxxxx32'	'2020-01-01 00:00:00.000'	false	0	0	0.23	0.23	'0xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx64'	'2020-01-01 00:00:00.000'
{.is-info}


### 3、查询数据

通过以下命令查询一定时间范围内的数据。

#### windows平台（find_table_general.bat）
> call "rtdb_import.exe" -find.table.general -engine rtdb -server 192.168.1.43:1234 -thread 80 -timeout.recv infinite /start_time '2020-01-01' -step_time 1000 -stop_time 5h -db DB_TEST_WRITE -path data/general_table.conf -format txt
{.is-info}

#### linux平台（find_table_general.sh）
> ./rtdb_import -find.table.general -engine rtdb -server 192.168.1.43:1234 -thread 5 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 5h -db DB_TEST_WRITE -path data/general_table.conf -format txt
{.is-info}

### 查询数据参数解释

  |   参数名称              | 参数说明            |        描述                           |
  | ----                  |  -                 |   ---                                |
  | find.table.general    | 执行查询数据操作      |                                       |
  | engine                | 执行哪种数据库引擎    | 目前分别支持: rtdb,taos,timescaledb     |
  | server                | 数据库服务的host url | 服务器ip地址与端口号                     |
  | thread                | 线程数              |                                      |
  | timeout.conn          | 网络连接超时时间      |  单位毫秒，  infinite：表示没有超时时间   |
  | timeout.send          | 发送网络消息超时时间   |  单位毫秒，  infinite：表示没有超时时间   |
  | timeout.recv          | 接收网络消息超时时间      |  单位毫秒，  infinite：表示没有超时时间   |
  | start_time            | 以什么时间作为查询数据的开始时间  |                          |
  | step_time             | 相邻两次查询之前的查询的开始时间的时间间隔 | 单位毫秒          |
  | stop_time             | 每次查询的时间范围  | 单位：（s:秒，m：分钟，h：小时，d：天，w：周）  |
  | path                  | 表的结构模板文件   |                                      |
  | format                | 表的结构模板文件的格式        | 目前只支持txt                 |
  | db                    | 数据库名称           |                                      |
   
注：将时间范围是以滚动向前的方式不断循环查询，每次查询的时间范围如下所示：
> 第一次查询的时间范围：\[start_time，start_time+stop_time\]
> 
> 第二次查询的时间范围：\[start_time+step_time，start_time+step_time+stop_time\]
> 
> 第n次查询的时间范围：\[start_time+step_time*（n-1），start_time+step_time*（n-1）+stop_time\]



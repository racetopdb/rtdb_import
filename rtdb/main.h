#ifndef _rtdb_test_wide_main_h_
#define _rtdb_test_wide_main_h_

#include "wide_base.h"
#include "insert_general.h"

namespace rtdb
{

namespace wide
{

int generate_table( int argc, char ** argv );
int create_table( int argc, char ** argv );
int create_table_v2( int argc, char ** argv );
int insert_table( int argc, char ** argv );
int insert_table_v2( int argc, char ** argv );
int find_table( int argc, char ** argv );
int find_table_v2( int argc, char ** argv );

int generate_data_general( int argc, char ** argv );
int create_table_general( int argc, char ** argv );
//int insert_table_general( int argc, char ** argv );
int find_table_general( int argc, char ** argv );

} // namespace wide

} // namespace rtdb

#endif

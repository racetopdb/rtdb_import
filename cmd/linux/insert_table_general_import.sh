#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./
./wide_rtdb -insert.table.general -engine rtdb -server 192.168.1.43:12345 -thread 40 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite  -start_time '2020-01-01' -step_time 1000  -stop_time 1h -sql_size 128k -path ./general_data.conf -format csv  -db DB_TEST_WRITE -table_conf ./general_table.conf

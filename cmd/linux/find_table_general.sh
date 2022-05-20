#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./
./rtdb_import -find.table.general -engine rtdb -server 192.168.1.43:1234 -thread 5 -timeout.recv infinite -start_time '2020-01-01' -step_time 1000 -stop_time 5h -db DB_TEST_WRITE -path data/general_table.conf -format txt

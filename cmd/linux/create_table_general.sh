#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./
./rtdb_import -create.table.general -engine rtdb -server 192.168.1.43:1234 -thread 80 -timeout.conn infinite -timeout.send infinite -timeout.recv infinite -path data/general_table.conf -format txt  -db DB_TEST_WRITE

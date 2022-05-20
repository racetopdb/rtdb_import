call "rtdb_import.exe" -find.table.general -engine rtdb -server 192.168.1.43:1234 -thread 80 -timeout.recv infinite /start_time '2020-01-01' -step_time 1000 -db DB_TEST_WRITE -path data/general_table.conf -format txt
pause

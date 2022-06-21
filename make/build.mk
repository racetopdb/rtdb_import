
RTDB_PATH   = ../../rtdb/RTDB
TAOS_PATH   = ../../rtdb/TAOS
TIMESCALEDB_PATH = ../../rtdb/TIMESCALEDB
CURL_PATH = ../../rtdb/CURL
CJSON_PATH = ../../rtdb/CJSON
OPENTSDB_PATH = ../../rtdb/OPENTSDB
INFLUXDB_PATH = ../../rtdb/INFLUXDB
OUTPUT_PATH  = ../../output

INCLUDE     = -I$(RTDB_PATH)/include -I$(TAOS_PATH)/include -I$(TIMESCALEDB_PATH)/include -I$(OPENTSDB_PATH)/include -I$(CURL_PATH)/include -I$(CJSON_PATH) -I$(INFLUXDB_PATH) -I../../rtdb/source -I../../rtdb/source/dir  -I../../rtdb/source/none 

RTDB_LIB    = 
TAOS_LIB    = -L$(TAOS_PATH)/$(PLATFORM) -ltaos
TIMESCALEDB_LIB    = -L$(TIMESCALEDB_PATH)/$(PLATFORM) -lpq
CURL_LIB    = -L$(CURL_PATH)/$(PLATFORM) -lcurl

LIB         = -lpthread -pthread -lrt -ldl $(RTDB_LIB) $(TAOS_LIB) $(TIMESCALEDB_LIB) $(CURL_LIB)

ifeq ($(debug), false)
CFLAGS      = -O3 $(FLAGS) $(CPU_FLAGS)
LIB_DIR     = $(RTDB_PATH)/$(PLATFORM)/Release
OUTPUT_DIR  = $(OUTPUT_PATH)/$(PLATFORM)/Release
else
CFLAGS      = -p -g $(FLAGS) -rdynamic
LIB_DIR     = $(RTDB_PATH)/$(PLATFORM)/Debug
OUTPUT_DIR  = $(OUTPUT_PATH)/$(PLATFORM)/Debug
endif


./o/%.o:                   ../../rtdb/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/RTDB_%.o:              ../../rtdb/RTDB/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/TAOS_%.o:              ../../rtdb/TAOS/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/TIMESCALEDB_%.o:       ../../rtdb/TIMESCALEDB/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/CJSON_%.o:       ../../rtdb/CJSON/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<


./o/OPENTSDB_%.o:       ../../rtdb/OPENTSDB/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/INFLUXDB_%.o:       ../../rtdb/INFLUXDB/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<


./o/rtdb_source_%.o:       ../../rtdb/source/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/rtdb_source_dir_%.o:   ../../rtdb/source/dir/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/rtdb_source_none_%.o:   ../../rtdb/source/none/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

wide_RTDB_OBJECTS =  \
	./o/RTDB_wide_rtdb.o	\
	./o/RTDB_wide_rtdb_conn.o	\
	./o/TAOS_wide_taos.o	\
	./o/TAOS_wide_taos_conn.o	\
	./o/TIMESCALEDB_wide_timescaledb.o	\
	./o/TIMESCALEDB_wide_timescaledb_conn.o	\
	./o/CJSON_cJSON.o	\
	./o/CJSON_cJSON_Utils.o	\
	./o/OPENTSDB_wide_opentsdb.o	\
	./o/OPENTSDB_wide_opentsdb_conn.o	\
	./o/INFLUXDB_wide_influxdb.o	\
	./o/INFLUXDB_wide_influxdb_conn.o	\
	./o/rtdb_source_source.o	\
	./o/rtdb_source_dir_dir_source.o	\
	./o/rtdb_source_dir_dir_worker.o	\
	./o/rtdb_source_none_none_source.o	\
	./o/rtdb_source_none_none_worker.o	\
	./o/create_table.o	\
	./o/create_table_v2.o	\
	./o/create_table_general.o	\
	./o/find.o	\
	./o/find_v2.o	\
	./o/find_general.o	\
	./o/generate.o	\
	./o/generate_general.o	\
	./o/insert.o	\
	./o/insert_v2.o	\
	./o/insert_general.o	\
	./o/main.o	\
	./o/utils.o	\
	./o/HTTP.o	\
	./o/file_operation.o	\
	./o/wide_base.o
	
all:
	mkdir -p ./o
	mkdir -p $(OUTPUT_DIR)
	$(MAKE) exe

exe: rtdb_import

rtdb_import: $(wide_RTDB_OBJECTS)

	rm -f $(OUTPUT_DIR)/wide_rtdb
	rm -f $(OUTPUT_DIR)/libtsdb.so
	rm -rf $(OUTPUT_DIR)/libpq.a
	rm -rf $(OUTPUT_DIR)/libpq.so
	rm -rf $(OUTPUT_DIR)/libpq.so.5
	rm -rf $(OUTPUT_DIR)/libpq.so.5.11
	rm -rf $(OUTPUT_DIR)/libcurl.a  
	rm -rf $(OUTPUT_DIR)/libcurl.la  
	rm -rf $(OUTPUT_DIR)/libcurl.so  
	rm -rf $(OUTPUT_DIR)/libcurl.so.4  
	rm -rf $(OUTPUT_DIR)/libcurl.so.4.8.0


	cp $(LIB_DIR)/libtsdb.so $(OUTPUT_DIR)/
	cp $(TAOS_PATH)/$(PLATFORM)/libtaos.so $(OUTPUT_DIR)/libtaos.so.1
#cp $(TIMESCALEDB_PATH)/$(PLATFORM)/libpq.a $(OUTPUT_DIR)/
	cp $(TIMESCALEDB_PATH)/$(PLATFORM)/libpq.so.5.11 $(OUTPUT_DIR)/libpq.so.5
#ln -s $(OUTPUT_DIR)/libpq.so.5.11 $(OUTPUT_DIR)/libpq.so
#ln -s $(OUTPUT_DIR)/libpq.so.5.11 $(OUTPUT_DIR)/libpq.so.5
	
	cp $(CURL_PATH)/$(PLATFORM)/libcurl.so.4.8.0  $(OUTPUT_DIR)/libcurl.so.4
	
	
	$(CPPC) -rdynamic $(LIB)				\
			$(wide_RTDB_OBJECTS)			\
			-o $(OUTPUT_DIR)/rtdb_import
       
	cp -r ../../cmd/data     $(OUTPUT_DIR)/ 
	cp -r ../../cmd/linux/*  $(OUTPUT_DIR)/
	chmod +x $(OUTPUT_DIR)/rtdb_import

clean:
	rm -fr ./o/*.o
	rm -fr $(wide_RTDB_OBJECTS)
	rm -f $(OUTPUT_DIR)/rtdb_import

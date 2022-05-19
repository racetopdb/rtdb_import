# wide_rtdb for CentOS 7.3
# make debug=true

RTDB_PATH   = RTDB
TAOS_PATH   = TAOS
TIMESCALEDB_PATH = TIMESCALEDB
MY_SOURCE_NAME   = source
MY_DIR_NAME      = source_dir
MY_NONE_NAME     = source_none

PLATFORM    = centos73

LIB_PATH    = $(RTDB_PATH)

OUTPUT_PATH  = .

debug       = false

CC          = gcc
CPPC        = g++

INCLUDE     = -I$(RTDB_PATH)/include -I$(TAOS_PATH)/include -I$(TIMESCALEDB_PATH)/include -I./source -I./source/dir  -I./source/none  
LIB_DIR     = $(LIB_PATH)/$(PLATFORM)
OUTPUT_DIR  = $(OUTPUT_PATH)

ifeq ($(debug), false)
CFLAGS      = -O3 $(FLAGS) $(CPU_FLAGS)
else
CFLAGS      = -g $(FLAGS) -D_DEBUG
endif

TSDB_LIB    = 
TAOS_LIB    = -L$(TAOS_PATH)/$(PLATFORM) -ltaos
TIMESCALEDB_LIB    = -L$(TIMESCALEDB_PATH)/$(PLATFORM) -lpq

LIB         = -lpthread -pthread -lrt -ldl $(TSDB_LIB) $(TAOS_LIB) $(TIMESCALEDB_LIB)
FLAGS       = -Wno-unknown-pragmas -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_LARGEFILE64 -fPIC -Wall -Wno-unused -Wno-deprecated
CPU_FLAGS   = -mtune=nocona -march=nocona -mfpmath=sse -m64 -mmmx -msse -msse2 -msse3 -Wall -Wno-unused

./o/%.o: ./%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/$(RTDB_PATH)_%.o: ./$(RTDB_PATH)/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/$(TAOS_PATH)_%.o: ./$(TAOS_PATH)/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/$(TIMESCALEDB_PATH)_%.o: ./$(TIMESCALEDB_PATH)/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<


./o/$(MY_SOURCE_NAME)_%.o: ./source/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/$(MY_DIR_NAME)_%.o: ./source/dir/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

./o/$(MY_NONE_NAME)_%.o: ./source/none/%.cpp
	$(CPPC)                $(CFLAGS) $(INCLUDE) -c -o $@ $<

wide_RTDB_OBJECTS=  \
	./o/$(RTDB_PATH)_wide_rtdb.o	\
	./o/$(RTDB_PATH)_wide_rtdb_conn.o	\
	./o/$(TAOS_PATH)_wide_taos.o	\
	./o/$(TAOS_PATH)_wide_taos_conn.o	\
	./o/$(TIMESCALEDB_PATH)_wide_timescaledb.o	\
	./o/$(TIMESCALEDB_PATH)_wide_timescaledb_conn.o	\
	./o/$(MY_SOURCE_NAME)_source.o	\
	./o/$(MY_DIR_NAME)_dir_source.o	\
	./o/$(MY_DIR_NAME)_dir_worker.o	\
	./o/$(MY_NONE_NAME)_none_source.o	\
	./o/$(MY_NONE_NAME)_none_worker.o	\
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
	./o/file_operation.o	\
	./o/wide_base.o
	
all:
	mkdir -p ./o
	mkdir -p $(OUTPUT_DIR)
	$(MAKE) exe

exe: wide_rtdb

wide_rtdb: $(wide_RTDB_OBJECTS)

	rm -f $(OUTPUT_DIR)/wide_rtdb
	rm -f $(OUTPUT_DIR)/libtsdb.so
	rm -rf $(OUTPUT_DIR)/libpq.a
	rm -rf $(OUTPUT_DIR)/libpq.so
	rm -rf $(OUTPUT_DIR)/libpq.so.5
	rm -rf $(OUTPUT_DIR)/libpq.so.5.11
	cp $(LIB_DIR)/libtsdb.so $(OUTPUT_DIR)/
	cp $(TAOS_PATH)/$(PLATFORM)/libtaos.so $(OUTPUT_DIR)/libtaos.so.1
#cp $(TIMESCALEDB_PATH)/$(PLATFORM)/libpq.a $(OUTPUT_DIR)/
	cp $(TIMESCALEDB_PATH)/$(PLATFORM)/libpq.so.5.11 $(OUTPUT_DIR)/libpq.so.5
#ln -s $(OUTPUT_DIR)/libpq.so.5.11 $(OUTPUT_DIR)/libpq.so
#ln -s $(OUTPUT_DIR)/libpq.so.5.11 $(OUTPUT_DIR)/libpq.so.5
	
	$(CPPC) -rdynamic $(LIB)				\
			$(wide_RTDB_OBJECTS)			\
			-o $(OUTPUT_DIR)/wide_rtdb

	chmod +x $(OUTPUT_DIR)/wide_rtdb

clean:
	rm -fr $(wide_RTDB_OBJECTS)
	rm -f $(OUTPUT_DIR)/wide_rtdb

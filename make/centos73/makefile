# tsdb for linux
# make debug=true platform=centos73


PLATFORM	= centos73

debug       = false

CC          = gcc
CPPC        = g++

FLAGS       = -Wno-unknown-pragmas -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_LARGEFILE64 -fPIC -Wall -Wno-unused -Wno-deprecated
CPU_FLAGS   = -mtune=nocona -march=nocona -mfpmath=sse -m64 -mmmx -msse -msse2 -msse3 -Wall -Wno-unused

ifeq ($(enable17), false)
ENABLE_C_PLUS_PLUS_17 = false
else
ENABLE_C_PLUS_PLUS_17 = true
endif

include ../build.mk


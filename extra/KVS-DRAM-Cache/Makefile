.SUFFIXES : .c .cc .o

PROJECT_PATH=/home/${USER}/projects
#PROJECT_BUILD=build-smaug-3
PROJECT_BUILD=build

#########CACHE_MODE#########
#0: default memcached + KVS
#1: (full) store whole data block with short-cut pointer
#2: (short) only store short-cut pointer
#3: hybrid mode (hot-cold)
#4: KVS only, no cache
#5: cache only, no KVS
CACHE_MODE=0
#########VERSION_MODE##########
#0: This number is used for partitioned mode that don't need version verification
#1: This number is used for version based sharing mode
#2: This number is used for version based sharing mode, and will force a read from FAM
VERSION_MODE=0


CC = gcc -std=gnu99 -DVERSION_MODE=${VERSION_MODE} -DCACHE_MODE=${CACHE_MODE}
CFLAGS = -g -O2 -Wall -lpthread -pthread -pedantic \
		 -Wmissing-prototypes -Wmissing-declarations
CXX = g++ -std=c++11 -DVERSION_MODE=${VERSION_MODE} -DCACHE_MODE=${CACHE_MODE}
CXXFLAGS = -g -O2 -Wall  \
			-pedantic -fpermissive -I${INCLUDE_KVS_PATH} -I${INCLUDE_NVMM_PATH}

LIB_FLAGS = -fPIC

C_LIB_OBJECTS = memcached.o assoc.o bipbuffer.o cache.o crawler.o\
		  daemon.o hash.o items.o itoa_ljust.o jenkins_hash.o\
		  logger.o murmur3_hash.o slabs.o slab_automove.o thread.o\
		  stats.o util.o

CXX_LIB_OBJECTS = cache_api.o

LIB_KVS_CACHE_PATH = ${PROJECT_PATH}/memcached
LIB_KVS_PATH = ${PROJECT_PATH}/radixtree/${PROJECT_BUILD}/src
LIB_NVMM_PATH = ${PROJECT_PATH}/nvmm/${PROJECT_BUILD}/src
INCLUDE_KVS_PATH = ${PROJECT_PATH}/radixtree/include
INCLUDE_NVMM_PATH = ${PROJECT_PATH}/nvmm/include
TARGET = libkvs_cache.so memcached_server # memcached_local kvs_local client

# ifeq (${MODE}, SERVER)
# 	TARGET = libkvs_cache.so memcached_server client
# else ifeq (${MODE}, LOCAL)
# 	TARGET = libkvs_cache.so memcached_local
# else ifeq (${MODE}, KVS)
# 	TARGET = kvs_local
# endif

all : $(TARGET)

$(C_LIB_OBJECTS): EXTRA_FLAGS := -fPIC

$(CXX_LIB_OBJECTS): EXTRA_FLAGS := -fPIC

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

libkvs_cache.so:  $(C_LIB_OBJECTS) ${CXX_LIB_OBJECTS}
	$(CXX) -shared -Wl,-soname,$@ -o $@ $(C_LIB_OBJECTS) ${CXX_LIB_OBJECTS}

client : client.o
	$(CXX) -g -O2 -lstdc++ -lpthread -lmemcached -Wl,-rpath=$(LIB_KVS_PATH)\
		-o $@ client.cc

memcached_server : server.o
	$(CXX) $(CXXFLAGS) -o $@ server.o\
		-L$(LIB_KVS_CACHE_PATH) -lkvs_cache\
		-L$(LIB_KVS_PATH) -lradixtree -lcluster\
		-L$(LIB_NVMM_PATH) -lnvmm\
		-Wl,-rpath=$(LIB_KVS_PATH)\
		-Wl,-rpath=$(LIB_NVMM_PATH)\
		-Wl,-rpath=${LIB_KVS_CACHE_PATH}\
		-levent -lpthread -pthread -lrt 


memcached_local : local.o
	$(CXX) $(CXXFLAGS) -o $@ local.o\
		-L$(LIB_KVS_CACHE_PATH) -lkvs_cache\
		-L$(LIB_KVS_PATH) -lradixtree\
		-L$(LIB_NVMM_PATH) -lnvmm\
		-Wl,-rpath=$(LIB_KVS_PATH)\
		-Wl,-rpath=$(LIB_NVMM_PATH)\
		-Wl,-rpath=${LIB_KVS_CACHE_PATH}\
		-levent -lpthread -pthread -lrt 


kvs_local : kvs_local.o
	$(CXX) $(CXXFLAGS) -o $@ kvs_local.cc\
		-L$(LIB_KVS_CACHE_PATH) -lkvs_cache\
		-L$(LIB_KVS_PATH) -lradixtree\
		-L$(LIB_NVMM_PATH) -lnvmm \
		-Wl,-rpath=$(LIB_KVS_PATH)\
		-Wl,-rpath=$(LIB_NVMM_PATH)\
		-Wl,-rpath=${LIB_KVS_CACHE_PATH}\
		-levent -lpthread -pthread -lrt 

dep:
	$(CC) -M ./*.c > .depend
	$(CXX) -M ./*.cc >> .depend

clean:
	rm ./*.o
	rm $(TARGET)

distclean: clean
	rm ./.depend


ifeq (.depend,$(wildcard .depend))
	include .depend
endif

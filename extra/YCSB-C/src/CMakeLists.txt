add_subdirectory(core)
add_subdirectory(db)

add_library(ycsb SHARED ${YCSB_SRC})
#target_link_libraries(ycsb tbb)
target_link_libraries(ycsb ${NVMM_LIBRARY} ${RADIXTREE_LIBRARY} ${KVS_CLIENT_LIBRARY} ${KVS_CACHE_LIBRARY})
target_link_libraries(ycsb ${ARCH_LIBS})

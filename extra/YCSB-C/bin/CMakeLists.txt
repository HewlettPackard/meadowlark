add_executable(ycsbc ycsbc.cc)
target_link_libraries(ycsbc ycsb pthread)


add_executable(micro micro.cc)
target_link_libraries(micro pthread ${NVMM_LIBRARY} ${RADIXTREE_LIBRARY} ${KVS_CACHE_LIBRARY})
target_link_libraries(micro ${ARCH_LIBS})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/workloads DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

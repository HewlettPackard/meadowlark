add_library(radixtree SHARED 
	failinj.cc 
	kvs_metrics_config.cc
	kvs_radix_tree.cc 
	kvs_radix_tree_tiny.cc 
	kvs_split_ordered.cc 
	kvs.cc 
	radix_tree.cc 
	split_ordered.cc)

target_link_libraries(radixtree ${NVMM_LIBRARY})
target_link_libraries(radixtree ${ARCH_LIBS})
target_link_libraries(radixtree cityhash)
target_link_libraries(radixtree ${MEDIDA_LIBRARY})
target_link_libraries(radixtree boost_program_options)

add_subdirectory(cluster)
add_library(cluster SHARED ${CLUSTER_SRC})
target_link_libraries(cluster yaml-cpp medida)

add_subdirectory(kvs_client)
add_library(kvs_client SHARED ${KVS_CLIENT_SRC})
add_dependencies(kvs_client libmemcached-1.0.18)
target_link_libraries(kvs_client cluster memcached pthread)

if(USE_QUARTZ)
  target_link_libraries(radixtree ${QUARTZ_LIBRARY})
endif()


add_subdirectory(cache)

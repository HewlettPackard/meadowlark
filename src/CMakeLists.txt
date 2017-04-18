add_library(radixtree SHARED radix_tree.cc radixtree_libpmem.cc split_list.cc kvs_radix_tree.cc kvs.cc)
target_link_libraries(radixtree ${NVMM_LIBRARY})
target_link_libraries(radixtree ${ARCH_LIBS})

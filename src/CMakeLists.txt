add_library(radixtree SHARED radix_tree.cc transaction.cc transaction_manager.cc status.cc radixtree_libpmem.cc)
target_link_libraries(radixtree boost_thread boost_log boost_log_setup boost_filesystem boost_system ${NVMM_LIB})
target_link_libraries(radixtree ${ARCH_LIBS})

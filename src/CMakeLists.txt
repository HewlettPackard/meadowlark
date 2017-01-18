add_library(radixtree SHARED RadixTree.cpp Transaction.cpp TransactionManager.cpp log.cc status.cc)
target_link_libraries(radixtree boost_thread boost_log boost_log_setup boost_filesystem boost_system ${NVMM_LIB})
target_link_libraries(radixtree ${ARCH_LIBS})

set(YCSB_SRC
  ${YCSB_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/core_workload.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/counter_generator.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/acked_counter_generator.cc
  PARENT_SCOPE
  )

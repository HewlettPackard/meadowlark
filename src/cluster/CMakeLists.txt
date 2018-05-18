set(CLUSTER_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/config.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/cluster.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/resource_manager.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/partition_manager.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/node_manager.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/server_manager.cc
)

set(CLUSTER_SRC "${CLUSTER_SRC}" PARENT_SCOPE)

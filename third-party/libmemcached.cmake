include(ExternalProject)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/libmemcached-1.0.18)
  message(STATUS "Copying libmemcached from source to build...")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/libmemcached-1.0.18.tar.gz
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

ExternalProject_Add(libmemcached-1.0.18
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libmemcached-1.0.18
  CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND make -j
  INSTALL_COMMAND make install
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libmemcached-1.0.18
  BUILD_IN_SOURCE 1
  )

add_library(libmemcached SHARED IMPORTED)
set_target_properties(libmemcached PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libmemcached-1.0.18/lib/libmemcached.so)

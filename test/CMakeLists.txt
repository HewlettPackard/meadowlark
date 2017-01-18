##########################
# Following are for gtest
##########################

function (add_radixtree_test file_name )
  add_executable(${file_name} ${file_name}.cpp)
  add_dependencies(${file_name} radixtree_shelf_base_dir)  
  target_link_libraries(${file_name} radixtree pthread gtest)
  add_test(NAME ${file_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${file_name})
endfunction()

add_radixtree_test(test_radix_tree)
add_radixtree_test(test_transaction)

function (add_demo file_name)
  add_executable(${file_name} ${file_name}.cc)
  add_dependencies(${file_name} radixtree_shelf_base_dir)
  target_link_libraries(${file_name} radixtree pthread boost_filesystem boost_system)
endfunction()

add_demo(demo_radix_tree)

AUX_SOURCE_DIRECTORY(. all_src)
add_executable(easywork ${all_src})
target_link_libraries(easywork z rt
                      boost_thread boost_system ev pthread)

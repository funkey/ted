define_module(pyted LIBRARY LINKS evaluation boost-python numpy)
add_custom_target(rename_pyted_lib ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/python/libpyted.so ${CMAKE_BINARY_DIR}/python/pyted.so)
add_dependencies(rename_pyted_lib pyted)

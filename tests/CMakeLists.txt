add_executable(layer_cut_engine_test
  engine_test.cpp
)

target_link_libraries(layer_cut_engine_test
  PRIVATE
    layer_cut_engine
)

add_test(NAME layer_cut_engine_test COMMAND layer_cut_engine_test)

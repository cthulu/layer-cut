add_executable(layer_cut_engine_test
  engine_test.cpp
)

target_link_libraries(layer_cut_engine_test
  PRIVATE
    layer_cut_engine
)

target_compile_definitions(layer_cut_engine_test
  PRIVATE
    LAYER_CUT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

add_test(NAME layer_cut_engine_test COMMAND layer_cut_engine_test)

add_executable(layer_cut_mesh_validation_test
  mesh_validation_test.cpp
)

target_link_libraries(layer_cut_mesh_validation_test
  PRIVATE
    layer_cut_engine
)

target_compile_definitions(layer_cut_mesh_validation_test
  PRIVATE
    LAYER_CUT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

add_test(NAME layer_cut_mesh_validation_test
  COMMAND layer_cut_mesh_validation_test
)

add_executable(layer_cut_plane_intersection_test
  plane_intersection_test.cpp
)

target_link_libraries(layer_cut_plane_intersection_test
  PRIVATE
    layer_cut_engine
)

add_test(NAME layer_cut_plane_intersection_test
  COMMAND layer_cut_plane_intersection_test
)

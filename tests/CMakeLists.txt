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

add_executable(layer_cut_contour_builder_test
  contour_builder_test.cpp
)

target_link_libraries(layer_cut_contour_builder_test
  PRIVATE
    layer_cut_engine
)

add_test(NAME layer_cut_contour_builder_test
  COMMAND layer_cut_contour_builder_test
)

add_executable(layer_cut_slicer_test slicer_test.cpp)
target_link_libraries(layer_cut_slicer_test PRIVATE layer_cut_engine)
target_compile_definitions(layer_cut_slicer_test
  PRIVATE
    LAYER_CUT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)
add_test(NAME layer_cut_slicer_test COMMAND layer_cut_slicer_test)

add_executable(layer_cut_svg_writer_test svg_writer_test.cpp)
target_link_libraries(layer_cut_svg_writer_test PRIVATE layer_cut_engine)
add_test(NAME layer_cut_svg_writer_test COMMAND layer_cut_svg_writer_test)

add_executable(layer_cut_stl_writer_test stl_writer_test.cpp)
target_link_libraries(layer_cut_stl_writer_test PRIVATE layer_cut_engine)
add_test(NAME layer_cut_stl_writer_test COMMAND layer_cut_stl_writer_test)

add_executable(layer_cut_stacked_preview_test stacked_preview_test.cpp)
target_link_libraries(layer_cut_stacked_preview_test PRIVATE layer_cut_engine)
add_test(NAME layer_cut_stacked_preview_test COMMAND layer_cut_stacked_preview_test)

add_executable(layer_cut_cricut_page_test cricut_page_test.cpp)
target_link_libraries(layer_cut_cricut_page_test PRIVATE layer_cut_engine)
add_test(NAME layer_cut_cricut_page_test COMMAND layer_cut_cricut_page_test)

if(TARGET stb_image_write)
  add_executable(layer_cut_png_writer_test png_writer_test.cpp)
  target_link_libraries(layer_cut_png_writer_test PRIVATE layer_cut_engine)
  add_test(NAME layer_cut_png_writer_test COMMAND layer_cut_png_writer_test)
endif()

add_executable(layer_cut_geometry_fixtures_test geometry_fixtures_test.cpp)
target_link_libraries(layer_cut_geometry_fixtures_test PRIVATE layer_cut_engine)
add_test(NAME layer_cut_geometry_fixtures_test COMMAND layer_cut_geometry_fixtures_test)

add_executable(layer_cut_c_api_test c_api_test.c)
target_link_libraries(layer_cut_c_api_test PRIVATE layer_cut_engine)
target_compile_definitions(layer_cut_c_api_test PRIVATE LAYER_CUT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
set_target_properties(layer_cut_c_api_test PROPERTIES LINKER_LANGUAGE CXX)
add_test(NAME layer_cut_c_api_test COMMAND layer_cut_c_api_test)

if(TARGET Clipper2)
  add_executable(layer_cut_polygon_ops_test polygon_ops_test.cpp)
  target_link_libraries(layer_cut_polygon_ops_test PRIVATE layer_cut_engine)
  add_test(NAME layer_cut_polygon_ops_test COMMAND layer_cut_polygon_ops_test)
endif()

if(TARGET layer-cut)
  add_test(NAME layer_cut_cli_svg_integration_test
    COMMAND ${CMAKE_COMMAND}
      -DCLI=$<TARGET_FILE:layer-cut>
      -DINPUT=${PROJECT_SOURCE_DIR}/testfiles/fox.stl
      -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-svg-output
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_integration.cmake
  )
endif()

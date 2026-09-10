add_library(layer_cut_engine STATIC
  src/engine.cpp
  src/mesh.cpp
  src/plane_intersection.cpp
  src/contour_builder.cpp
  src/slicer.cpp
  src/svg_writer.cpp
  src/png_writer.cpp
  src/stl_writer.cpp
  src/stacked_preview.cpp
  src/cricut_page.cpp
  src/stl_loader.cpp
  src/transform.cpp
  src/layer_cut.cpp
)

target_include_directories(layer_cut_engine
  PUBLIC
    ${PROJECT_SOURCE_DIR}/engine/include
)

target_link_libraries(layer_cut_engine
  PRIVATE
    layer_cut_warnings
)

if(TARGET layer_cut_sanitizers)
  target_link_libraries(layer_cut_engine PUBLIC layer_cut_sanitizers)
endif()

if(TARGET Clipper2)
  target_sources(layer_cut_engine PRIVATE src/polygon_ops.cpp)
  target_link_libraries(layer_cut_engine PUBLIC Clipper2)
  target_compile_definitions(layer_cut_engine PRIVATE LAYER_CUT_HAVE_CLIPPER=1)
endif()

if(TARGET stb_image_write)
  target_compile_definitions(layer_cut_engine PRIVATE LAYER_CUT_HAVE_STB=1)
  target_link_libraries(layer_cut_engine PRIVATE stb_image_write)
endif()

set_target_properties(layer_cut_engine PROPERTIES
  OUTPUT_NAME layer-cut-engine
)

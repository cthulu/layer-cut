add_library(layer_cut_engine STATIC
  src/engine.cpp
  src/mesh.cpp
  src/plane_intersection.cpp
  src/contour_builder.cpp
  src/slicer.cpp
  src/stl_loader.cpp
)

target_include_directories(layer_cut_engine
  PUBLIC
    ${PROJECT_SOURCE_DIR}/engine/include
)

target_link_libraries(layer_cut_engine
  PUBLIC
    layer_cut_warnings
)

if(TARGET layer_cut_sanitizers)
  target_link_libraries(layer_cut_engine PUBLIC layer_cut_sanitizers)
endif()

if(TARGET Clipper2)
  target_sources(layer_cut_engine PRIVATE src/polygon_ops.cpp)
  target_link_libraries(layer_cut_engine PUBLIC Clipper2)
endif()

set_target_properties(layer_cut_engine PROPERTIES
  OUTPUT_NAME layer-cut-engine
)

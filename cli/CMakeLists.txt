add_executable(layer-cut main.cpp)

target_include_directories(layer-cut PRIVATE
  ${PROJECT_SOURCE_DIR}/engine/include
  ${cli11_SOURCE_DIR}/include
)

target_link_libraries(layer-cut
  PRIVATE
    layer_cut_engine
)

if(TARGET layer_cut_sanitizers)
  target_link_libraries(layer-cut PRIVATE layer_cut_sanitizers)
endif()

set_target_properties(layer-cut PROPERTIES
  OUTPUT_NAME layer-cut
)

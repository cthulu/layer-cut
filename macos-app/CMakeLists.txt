cmake_minimum_required(VERSION 3.20)

project(LayerCutApp LANGUAGES Swift)

if(NOT TARGET layer_cut_engine)
  message(FATAL_ERROR "LayerCutApp must be configured from the repository root so the CMake engine target is available")
endif()

# Swift UI target
add_executable(LayerCutApp
  Sources/LayerCutApp/LayerCutApp.swift
  Sources/LayerCutApp/EngineBridge.swift
  Sources/LayerCutApp/EngineServices.swift
  Sources/LayerCutApp/ExportWorkflow.swift
  Sources/LayerCutApp/ViewportView.swift
  Sources/LayerCutApp/Profile.swift
  Sources/LayerCutApp/ParametersPanel.swift
  Sources/LayerCutApp/LayerPreview.swift
)

target_link_libraries(LayerCutApp
  PRIVATE
    layer_cut_engine
)

target_include_directories(LayerCutApp
  PRIVATE
    ${PROJECT_SOURCE_DIR}/../engine/include
)

target_link_libraries(LayerCutApp
  PRIVATE
    "-framework SwiftUI"
    "-framework RealityKit"
    "-framework WebKit"
)

# Set macOS deployment target (RealityKit requires macOS 14+).
set_target_properties(LayerCutApp PROPERTIES
  MACOSX_BUNDLE YES
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.layercut.app"
  XCODE_ATTRIBUTE_PRODUCT_NAME "LayerCut"
  XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0"
  XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_SOURCE_DIR}/../engine/include/layer_cut.h"
  XCODE_GENERATE_SCHEME ON
  XCODE_SCHEME_DEBUG_ENTITLEMENTS OFF
  CODE_SIGN_IDENTITY "-"
  CODE_SIGNING_REQUIRED OFF
  CODE_SIGNING_ALLOWED OFF
)

if(BUILD_TESTING)
  add_executable(LayerCutProfileTests
    Sources/LayerCutApp/Profile.swift
    Tests/ProfileTests.swift
  )
  target_link_libraries(LayerCutProfileTests PRIVATE
    "-framework SwiftUI"
  )
  set_target_properties(LayerCutProfileTests PROPERTIES
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0"
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.9"
  )
  add_test(NAME layer_cut_profile_tests COMMAND LayerCutProfileTests)

  add_executable(LayerCutExportWorkflowTests
    Sources/LayerCutApp/EngineBridge.swift
    Sources/LayerCutApp/EngineServices.swift
    Sources/LayerCutApp/Profile.swift
    Sources/LayerCutApp/ExportWorkflow.swift
    Tests/ExportWorkflowTests.swift
  )
  target_link_libraries(LayerCutExportWorkflowTests PRIVATE
    layer_cut_engine
    "-framework SwiftUI"
  )
  set_target_properties(LayerCutExportWorkflowTests PROPERTIES
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0"
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.9"
  )
  add_test(NAME layer_cut_export_workflow_tests COMMAND LayerCutExportWorkflowTests)
endif()

set_target_properties(LayerCutApp PROPERTIES
  XCODE_ATTRIBUTE_SWIFT_VERSION "5.9"
)

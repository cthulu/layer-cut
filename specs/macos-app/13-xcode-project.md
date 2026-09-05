STEP 13 — Swift package / Xcode project setup
  Goal: Xcode project that links the C++ engine library.
  Action: macos-app/ with LayerCutApp.swift, Info.plist
  Tech:
    - Choose one reproducible integration model: Xcode project or Swift
      Package with a native/binary target.
    - Link libengine.a plus libc++, set deployment target and architecture,
      and verify arm64 (and x86_64 if supported).
    - Bridge header: #include "layer_cut.h"
    - Swift interop: Use UnsafePointer<CChar> for C string args and import the
      C header with fixed-width integer includes and documented ownership.
    - AI prompt suggestion:
      "Create a SwiftUI macOS app that links a static C++ library
       via a C ABI bridge header"

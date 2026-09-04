STEP 13 — Swift package / Xcode project setup
  Goal: Xcode project that links the C++ engine library.
  Action: macos-app/ with CricutSlicerApp.swift, Info.plist
  Tech:
    - Xcode project (.xcodeproj) or Swift Package with native target
    - Link libengine.a via Build Phases → Link Binary With Libraries
    - Bridge header: #include "cricut_slicer.h"
    - Swift interop: Use UnsafePointer<CChar> for C string args
    - AI prompt suggestion:
      "Create a SwiftUI macOS app that links a static C++ library
       via a C ABI bridge header"

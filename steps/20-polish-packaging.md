STEP 20 — Polish & packaging
  Goal: App ready for distribution.
  Action: Finalize UI, add settings, code sign, create .dmg.
  Tech:
    - Settings: default layer height, output format, DPI for PNG
    - Dark mode support (SwiftUI automatic)
    - Code sign: codesign --sign "Developer ID" CricutSlicer.app
    - Create DMG: hdiutil create -format ADIF
                   -srcfolder CricutSlicer.app cricut-slicer.dmg
    - Optional: notarization via xcrun notarytool

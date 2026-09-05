STEP 20 — Polish & packaging
  Goal: App ready for distribution.
  Action: Finalize UI, add settings, code sign, create .dmg.
  Tech:
    - Settings: default layer height, output format, DPI for PNG
    - Dark mode support (SwiftUI automatic)
    - Code sign: codesign --sign "Developer ID" LayerCut.app
    - Create DMG: hdiutil create -format ADIF
                    -srcfolder LayerCut.app layer-cut.dmg
    - Include hardened runtime, entitlements, architecture/universal-binary
      decision, reproducible versioning, and notarization prerequisites.

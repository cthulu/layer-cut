STEP 14 — File import UI
  Goal: User can drag-drop or browse for an STL file.
  Action: macos-app/ImportView.swift
  Tech:
    - SwiftUI FileImporter or DropZone (drag-and-drop overlay)
    - Validate file extension .stl
    - Show file size and basic info (triangles, bounding box)
    - Error handling: invalid STL, corrupted file, too large (>50MB)

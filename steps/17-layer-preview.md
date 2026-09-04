STEP 17 — Layer preview panel
  Goal: Visual preview of sliced layers (scrollable thumbnails).
  Action: macos-app/LayerPreviewView.swift
  Tech:
    - ScrollView with LazyVGrid of layer thumbnails
    - SVG rendered via PDFDocument → NSImage (SwiftUI Image(nsImage:))
    - PNG rendered directly via NSImage
    - Click a layer → highlight in 3D viewport
    - Layer count badge, zoom controls

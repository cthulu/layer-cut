STEP 17 — Layer preview panel
  Goal: Visual preview of sliced layers (scrollable thumbnails).
  Action: macos-app/LayerPreviewView.swift
  Tech:
    - ScrollView with LazyVGrid of layer thumbnails
    - Request engine-generated PNG preview bytes at a UI-appropriate DPI and
      render them directly via NSImage. This keeps preview geometry aligned
      with PNG export without adding an SVG renderer dependency.
    - Click a layer → highlight in 3D viewport
    - Layer count badge, zoom controls

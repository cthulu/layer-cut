STEP 19 — Export / save workflow
  Goal: User exports one automatically numbered, physically sized file per
    layer into the chosen output directory.
  Action: macos-app/ExportView.swift
  Tech:
    - FileImporter with directory permission
    - Show output directory path, file count, total size, and the deterministic
      naming pattern (for example `layer_001.svg`).
    - "Export" button triggers batch write
    - Progress bar during export
    - Success/error toast notifications

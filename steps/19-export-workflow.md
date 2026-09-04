STEP 19 — Export / save workflow
  Goal: User exports sliced files to a chosen directory.
  Action: macos-app/ExportView.swift
  Tech:
    - FileImporter with directory permission
    - Show output directory path, file count, total size
    - "Export" button triggers batch write
    - Progress bar during export
    - Success/error toast notifications

# Layer Cut App Guardrails

These rules apply to all macOS app implementation work.

## Engine Boundary

- SwiftUI and app services provide presentation, user input, persistence, and
  orchestration only.
- The engine is authoritative for slicing, bounds, geometry, packing, rotation,
  numbering, page layout, path budgets, and derived export data.
- Do not parse STL files, reimplement geometry, calculate packing, calculate label
  positions, or duplicate engine algorithms in the UI.
- If a requested behavior appears to require a calculation in the UI layer, ask
  before implementing it there. Prefer adding the calculation to the engine or
  exposing the required result through the C ABI.

## Numeric Formatting

- Use C-style numeric formatting for displayed and persisted numeric values.
- The decimal separator is always `.`.
- Do not rely on the user's locale for profile files, CLI-facing values, SVG
  metadata, or diagnostic values.
- Preserve deterministic precision and serialization rules established by the
  engine contracts.

## Controls

- When a slider is needed, use the established two-row layout: label and editable
  value on the first row, slider on the second row.
- Keep controls thin and profile-driven; do not add UI-only derived settings.
- Use text in addition to color or icons for warnings, errors, loading, and empty
  states.

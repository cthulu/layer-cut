# Step 15 - Design Space Validation Record

Priority: P0 validation artifact

## Goal

Record real Design Space behavior using the five-slice sample before production
export behavior is declared stable.

## Checklist

- Upload both cut and guide SVGs.
- Confirm the page retains physical millimetre dimensions.
- Confirm both imported documents contain the shared alignment marker and that
  its geometry overlays exactly when the documents are stacked.
- Confirm group names are visible or identify the fallback selection workflow.
- Confirm groups can be ungrouped and independently selected.
- Change a guide group to `Draw > Pen`.
- Attach the guide group to the corresponding cut group.
- Plot the guide on the previous slice.
- Confirm negative-gap overlap warning behavior in the generated output.
- Confirm the SVG path limit behavior.
- Record desktop and mobile differences if tested.

## Automated Acceptance Tests

- Parse each paired cut and guide SVG and assert that the alignment marker
  geometry has identical coordinates and attributes in both files.
- Assert that the marker is emitted once per paired page and is included in the
  path budget.

## Manual Alignment Test

Import the cut and guide SVGs as separate Design Space documents, use the shared
marker to align them, and verify that the page and layer geometry overlay without
manual size or offset correction. Record any import scaling or alignment drift.

## Required Outcome

Update the implementation specs with any Design Space behavior that differs from
standard SVG expectations, especially group naming, metadata preservation, and
operation assignment.

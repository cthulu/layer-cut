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
- Assign the guide marker to Pen and leave the cut marker out of the Cut
  operation.
- Confirm the observed fallback: imported groups may be displayed only as
  generic `Group` entries. Use stable ordering, visual styling, filenames, and
  the alignment marker instead of relying on SVG group names.
- Confirm groups can be ungrouped and independently selected.
- Change a guide group to `Draw > Pen`.
- Attach the guide group to the corresponding cut group.
- Plot the guide on the previous slice.
- Confirm negative-gap overlap warning behavior in the generated output.
- Confirm the SVG path limit behavior.
- Record desktop and mobile differences if tested.

## Automated Acceptance Tests

Do not automate Design Space itself. Repository tests may compare paired SVG
marker geometry, but import, operation assignment, stacking, and plotting remain
manual checks.

## Manual Alignment Test

Import the cut and guide SVGs as separate Design Space documents, use the shared
marker to align them, and verify that the page and layer geometry overlay without
manual size or offset correction. Record any import scaling or alignment drift.

## Required Outcome

Observed behavior: Design Space does not display the SVG group names and labels
them `Group`. This is acceptable for the current workflow. Group names and SVG
metadata remain useful for diagnostics, but must not be required for operation.
The shared marker successfully aligns the cut and guide documents. The guide
marker is assigned to Pen and the cut marker is excluded from Cut manually.
Update future UI/export documentation if Design Space changes this behavior.

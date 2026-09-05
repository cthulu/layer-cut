# Step 15 - Design Space Validation Record

Priority: P0 validation artifact

## Goal

Record real Design Space behavior using the five-slice sample before production
export behavior is declared stable.

## Checklist

- Upload both cut and guide SVGs.
- Confirm the page retains physical millimetre dimensions.
- Confirm group names are visible or identify the fallback selection workflow.
- Confirm groups can be ungrouped and independently selected.
- Change a guide group to `Draw > Pen`.
- Attach the guide group to the corresponding cut group.
- Plot the guide on the previous slice.
- Confirm negative-gap overlap warning behavior in the generated output.
- Confirm the SVG path limit behavior.
- Record desktop and mobile differences if tested.

## Required Outcome

Update the implementation specs with any Design Space behavior that differs from
standard SVG expectations, especially group naming, metadata preservation, and
operation assignment.

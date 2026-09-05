# Step 02 - ASCII STL Writer

Priority: P1

## Goal

Serialize the stacked preview mesh as a portable ASCII STL file.

## Requirements

- Emit a valid `solid` header and `endsolid` footer.
- Emit one facet per triangle.
- Emit finite decimal coordinates.
- Emit deterministic formatting and triangle order.
- Compute or validate facet normals.
- Reject an empty or invalid mesh.
- Return structured file and serialization errors.
- Keep units documented as millimetres.

## Tests

Cover a cube, a holed prism, a concave prism, disconnected components, empty
meshes, invalid coordinates, and file write failures. Reopen generated files
with a test parser and verify triangle count and bounds.

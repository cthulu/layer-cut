STEP 3 — STL binary parser
  Goal: Parse binary STL files into a triangle mesh.
  Action: engine/src/stl_loader.cpp + engine/include/stl_loader.h
  Tech:
    - Binary STL: 80-byte header + uint32 triangle count +
      3×float (vertex) × N triangles + 2-byte attribute byte count.
    - Validate file length before allocation, detect truncation and integer
      overflow, require finite coordinates, and enforce configurable byte,
      triangle, and memory limits.
    - Version 1 accepts binary STL only. Detect ASCII STL and report it as
      unsupported rather than interpreting it as binary.
    - Output: std::vector<Triangle>
    - Unit tests cover valid, truncated, oversized, non-finite, and malformed
      files, including a file whose header begins with `solid`.

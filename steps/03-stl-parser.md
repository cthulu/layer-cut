STEP 3 — STL binary parser
  Goal: Parse binary STL files into a triangle mesh.
  Action: engine/src/stl_loader.cpp + engine/include/stl_loader.h
  Tech:
    - Binary STL: 80-byte header + 4×uint32 (normal) +
      3×float (vertex) × N triangles + 2-byte attr byte count
    - Output: std::vector<Triangle>
    - Validate triangle count ≤ 50M (memory guard)
    - Unit test: Parse known STL, verify vertex count & bbox

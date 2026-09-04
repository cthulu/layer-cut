STEP 4 — Triangle mesh data structure
  Goal: In-memory representation of the 3D model.
  Action: engine/include/mesh.h
  Tech:
    struct Vec3 { float x, y, z; };
    struct Triangle { Vec3 a, b, c; Vec3 normal; };
    struct Mesh {
        std::vector<Triangle> triangles;
        Vec3 min, max;  // bounding box
        float compute_volume() const;
    };

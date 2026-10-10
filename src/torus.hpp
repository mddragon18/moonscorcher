#pragma once
#include <array>
#include <vector>
#include <cstddef>

namespace geo {
struct TorusMesh {
  std::vector<float> vertices;     // pos(3) + normal(3), interleaved
  std::vector<float> ring_dist;    // per-vertex 0..1 around tube (for fur field)
};

TorusMesh make_torus(float R, float r, int seg_major, int seg_minor);
size_t vertex_count(const TorusMesh& m);
}  // namespace geo

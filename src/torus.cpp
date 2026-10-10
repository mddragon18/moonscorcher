// Torus + fur geometry (original implementation).
#include "torus.hpp"
#include <cmath>

namespace geo {

static void push(std::vector<float>& v, float a, float b, float c) {
  v.push_back(a); v.push_back(b); v.push_back(c);
}

TorusMesh make_torus(float R, float r, int seg_major, int seg_minor) {
  TorusMesh m;
  m.vertices.reserve(size_t(seg_major * seg_minor) * 6 * 2);
  m.ring_dist.reserve(size_t(seg_major * seg_minor));

  // Positions + normals for each grid vertex, then triangles with a marker
  // attribute (distance around tube) used by the fur field shader.
  for (int i = 0; i < seg_major; ++i) {
    for (int j = 0; j < seg_minor; ++j) {
      float u0 = float(i) / seg_major * 2.f * float(M_PI);
      float u1 = float(i + 1) / seg_major * 2.f * float(M_PI);
      float v0 = float(j) / seg_minor * 2.f * float(M_PI);
      float v1 = float(j + 1) / seg_minor * 2.f * float(M_PI);

      auto P = [R, r](float u, float v) {
        float cR = R + r * std::cos(v);
        return std::array<float,3>{ cR * std::cos(u), cR * std::sin(u), r * std::sin(v) };
      };
      auto N = [](float u, float v) {
        return std::array<float,3>{ std::cos(v) * std::cos(u), std::cos(v) * std::sin(u), std::sin(v) };
      };

      std::array<std::array<float,3>,2> us = { {u0,u1} };
      std::array<std::array<float,3>,2> vs = { {v0,v1} };
      int quads[4][2] = { {0,0},{1,0},{1,1},{0,1} };
      int tri[6] = { 0,1,2, 0,2,3 };
      for (int t : tri) {
        float u = us[quads[t][0]][0];
        float v = vs[quads[t][1]][0];
        auto p = P(u, v);
        auto n = N(u, v);
        push(m.vertices, p[0], p[1], p[2]);
        push(m.vertices, n[0], n[1], n[2]);
        m.ring_dist.push_back(v / (2.f * float(M_PI)));
      }
    }
  }
  return m;
}

}  // namespace geo

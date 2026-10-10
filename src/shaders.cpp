// Original fur-shim shell shaders (GLSL 330). No proprietary shader code.
#include "shaders.hpp"

namespace shaders {

const char* shell_vertex = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in float aRingDist;

uniform float uShell;        // 0..1 shell layer
uniform float uTime;
uniform float uFurLen;       // max fur length (world units)
uniform float uSpeed;

out vec3 vNormal;
out vec3 vPos;
out float vRingDist;
out float vShell;

// hash noise (public-domain style integer hash)
float hash(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
  // displacement amount: more for lower shells, modulated by a noise field
  float field = hash(floor(aPos * 14.0) + floor(vec3(uTime)));
  float grow = uShell * uFurLen * (0.65 + 0.35 * sin(uSpeed * uTime + field * 6.2831));
  vec3 p = aPos + aNormal * grow;
  vNormal = aNormal;
  vPos = p;
  vRingDist = aRingDist;
  vShell = uShell;
  gl_Position = vec4(p, 1.0);
}
)GLSL";

const char* shell_fragment = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vPos;
in float vRingDist;
in float vShell;

uniform vec3 uBase;      // fur color
uniform vec3 uBkg;       // clear color
out vec4 outColor;

float hash(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
  // alpha-test: keep fewer pixels as we reach fur tips
  float keep = 1.0 - vShell * vShell;
  float n = hash(floor(vPos * 220.0));
  if (n > keep) discard;

  // lighting: fixed key light + rim, fur shade darkens toward tips slightly
  vec3 L = normalize(vec3(0.4, 0.7, 0.6));
  float diff = max(dot(vNormal, L), 0.0);
  float rim = pow(1.0 - abs(dot(vNormal, vec3(0,0,1))), 2.0);
  vec3 col = uBase * (0.30 + 0.60 * diff) + rim * 0.15;
  col = mix(col, uBase * 0.55, vShell * 0.35);   // tips slightly darker
  outColor = vec4(col, 1.0);
}
)GLSL";

}  // namespace shaders

// Original fur-shim shell shaders (GLSL 330, mvp + burn-in saturator).
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
uniform float uScale;
uniform mat4  uModel;

out vec3 vNormal;
out vec3 vPos;
out float vRingDist;
out float vShell;

float hash(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
  float field = hash(floor(aPos * 14.0) + floor(vec3(uTime)));
  float grow = uShell * uFurLen * (0.65 + 0.35 * sin(uSpeed * uTime + field * 6.2831));
  vec3 p_local = (aPos + aNormal * grow) * uScale;
  vec3 p_tumble = (uModel * vec4(p_local, 1.0)).xyz;
  vNormal = mat3(uModel) * aNormal;
  vPos = p_tumble;
  vRingDist = aRingDist;
  vShell = uShell;
  // perspective-ish projection: distance attenuation, NDC fill tuned for scale 1
  float w = 1.6 - 0.25 * p_tumble.z;
  gl_Position = vec4(p_tumble.xy * (1.0 / w) * 0.9, p_tumble.z * 0.5, 1.0);
}
)GLSL";

const char* shell_fragment = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vPos;
in float vRingDist;
in float vShell;

uniform vec3 uBase;
uniform vec3 uBkg;
out vec4 outColor;

float hash(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// burn-in saturator: deterministic heavy FMA chain, no memory, no branches.
// W = 0 returns c unchanged-ish; W = 1 fully graded. Cost is constant per pixel.
vec3 saturator(vec3 c, float W) {
  vec3 a = c * 0.731 + 0.117;
  a = a * a + vec3(0.211, 0.109, 0.301);
  a = normalize(a + c * 0.153);
  a = a * a * a + vec3(0.417, 0.723, 0.911);
  a = normalize(a) * 0.913 + c * 0.207;
  a = a * a + vec3(0.113, 0.419, 0.701);
  a = normalize(a + c * 0.341);
  a = a * a * a + vec3(0.513, 0.329, 0.117);
  a = normalize(a) * 0.799 + c * 0.301;
  a = a * a + vec3(0.907, 0.213, 0.501);
  a = normalize(a + c * 0.419);
  a = a * a * a + vec3(0.117, 0.913, 0.727);
  a = normalize(a) * 0.887 + c * 0.421;
  a = a * a + vec3(0.301, 0.707, 0.319);
  a = normalize(a + c * 0.501);
  return mix(c, a, W);
}

void main() {
  float keep = 1.0 - vShell * vShell;
  float n = hash(floor(vPos * 220.0));
  if (n > keep) discard;

  vec3 L = normalize(vec3(0.4, 0.7, 0.6));
  float diff = max(dot(vNormal, L), 0.0);
  float rim = pow(1.0 - abs(dot(vNormal, vec3(0,0,1))), 2.0);
  vec3 col = uBase * (0.30 + 0.60 * diff) + rim * 0.15;
  col = mix(col, uBase * 0.55, vShell * 0.35);
  // heavy grade: fur tips carry most of the saturator work
  vec3 graded = saturator(col, 0.45 + 0.55 * vShell);
  outColor = vec4(graded, 1.0);
}
)GLSL";

}  // namespace shaders

# MoonScorcher — clean-room GPU stress/burn-in tool. Binary-derived behavior spec.

Derived exclusively from (a) observing stock FurMark v2.10.2 (Linux build) behavior and
outputs, and (b) private binary analysis done with Ghidra/REA for understanding only.
No proprietary code, shaders, or assets are copied. Implementation is original.

## Observed behavior of the original (spec target)

### Identity params recovered from binary strings/config (observed, public)
- app codename "furmark", vendor "Geeks3D", version 2.10.2.0
- benchmark_duration_ms default: 60000
- scores server: www.gpumagick.com, submit via /scores/s2.php with a UA string
- logging: `_furmark_log.txt` (and `_geexlab_log.txt`) in app dir
- background image / logo PNG / Xolonium-Bold font — proprietary assets, NOT copied.
  MoonScorcher uses original assets generated in code.

### Rendering pattern (publicly observable)
- Classic "furry donut": the torus rendered with GPU-synthesized fur.
  Multiple backends: `furmark-gl` (OpenGL 3.2), `furmark-vk` (Vulkan), `furmark-knot-gl`.
- Plugin toggles clarified by binary param sets: vulkan2 plugin on/off depending on mode;
  physx4/opencl/cuda/imagemagick plugins disabled for all furmark modes.
- GPU monitoring plugin (`gpumon`) enabled; logs GPU sensor lines to CSV:
  "GPU data log file. Default filename is `_gpu_date.time.csv`", "Default: -1 (all GPUs)".

### CLI surface (observed from --help strings)
- `--furmark-bkg-img filename`, `--furmark-bkg-img-id number`, `--furmark-tint-color`,
  `--furmark-bg-type`, `--furmark-bfc` (background fixed color?)
- DXT5 compressed textures toggle mention ("enable compressed textures (DXT5)").
- GPU selection: "Default: -1 (all GPUs)" => per-GPU log file selection support.
- Full CLI reference to be re-derived from `--help` output at runtime (fair game:
  it is user-facing behavior, not internal code).

## MoonScorcher design (reimplementation)

- Language: C++20. Windowing+GL: GLFW 3.5 (installed) + OpenGL 3.3 core for v1;
  Vulkan backend v2. No engine dependency — single-binary, no AppImage.
- Fur rendering (original technique, same visual class):
  - Geometry: parametric torus mesh, de Casteljau-free analytic (p,q) torus.
  - Fur: instanced shim shells (~24 shells), per-instance fur length animated by a
    time-varying field (sin(t*speed + dist-field)); alpha-tested shells with
    per-shell thickness; fuzzy noise in shell shader via hash-based noise.
  - Motion: slow torus tumble (the original's "knot" rotation), camera fixed.
- Benchmark mode: default 60s run, reports score in points = fps * 1000-like metric;
  writes `moonscorcher_date.time.csv` with per-second GPU stats from NVML (libnvidia-ml)
  plus timestamped log to `_moonscorcher_log.txt` mirroring the observed layout.
- CLI parity: `--duration-ms`, `--backend gl|vk`, `--gpu N`, `--bkg-img`, `--bkg-color`,
  `--windowed|--fullscreen`, `--score-log-path`. Extensible, no scores-server.
- No proprietary string, asset, or URL is reproduced.

## Parity test plan (against stock original, both free to run)
1. FPS-range sanity: same scene settings on same GPU, expect comparable fps class.
2. CSV output shape: same columns family (time, GPU temp, power, clocks, fan, fps/marks).
3. Temp/power trajectory: sustained load profile, per-second stats.
4. Exit behavior: prints final score line to log at duration end.

## Non-goals
- No scores submission to any external server.
- No reproduction of Geeks3D branding or assets.

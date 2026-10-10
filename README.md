# MoonScorcher

An open-source, clean-room GPU burn-in benchmark for Linux — inspired by the
behavior of the classic "furry donut" GPU stress tools. Built from scratch in
C++20 from observed behavior; no proprietary code, shaders, or assets.

```
cd /home/md/Projects/moonscorcher
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/moonscorcher --fullscreen --duration-ms 60000
```

## First verified run (RTX 3060, 2026-10-10, Wayland)

- Duration 60s, 49 samples @ ~1Hz
- Avg fps 49.56 (vsync-locked by compositor), score 49558 points
- GPU: 64°C start → 82°C peak steady-state
- Power: ~78W first second (clock ramp) → ~158–170W sustained

CSV written to `scratch/bench60.csv`; log appended to `_moonscorcher_log.txt`.

## Backends

- OpenGL 3.3 core (GLFW3, vendored glad loader) — shipped.
- Vulkan — planned.
- CPU-parallel fallback — planned, for non-NVIDIA GPUs via OpenCL or SYCL.

## Telemetry

NVML (libnvidia-ml.so.1) loaded dynamically at runtime: per-second CSV with
`time_s,fps,marks_s,gpu_temp_c,gpu_power_w`. Multi-GPU via `--gpu N`.

## License

MIT OR Apache-2.0.

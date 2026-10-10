# MoonScorcher

MoonScorcher is a GPU burn-in benchmark for Linux. You run it to put a heavy and
constant load on a GPU. This tells you if the GPU stays fast when it is hot, and
if the cooling system works.

MoonScorcher is a clean-room project. The authors looked at how FurMark behaves,
wrote a description of that behavior, and then made a new program from that
description. MoonScorcher does not contain FurMark code, shaders, or data files.

## Status

MoonScorcher works. It is not as complete as FurMark. This section tells you
what works and what does not work yet.

Works:

- It draws a fur-covered torus with an OpenGL 3.3 core context.
- It shows the FPS on samples of 1 second.
- It reads the GPU temperature and power draw with NVML.
- It records the temperature and the power draw of 1 second in a CSV file.
- It gives presets for 1080p, 1440p, 3440x1440, and 2160p.

Does not work yet:

- No Vulkan backend.
- No graphical interface. You must use a terminal.
- No CPU burn-in test.
- No score website.

## Build

You need these packages on Arch Linux: `cmake`, `glfw`, `mesa`, and a C++20
compiler. You need an NVIDIA GPU and the NVIDIA driver for telemetry. Other GPUs
work but give no temperature data.

To build MoonScorcher, do this:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Quick start

To run the 1080p benchmark, do this:

```
./build/moonscorcher --p1080
```

The benchmark runs for 60 seconds. Then MoonScorcher shows the score on the
terminal and writes two files:

- `_moonscorcher_log.txt` — the score and the GPU name.
- `_moonscorcher_<date>.csv` — the temperature, the power draw, and the FPS of
  each second.

## Options

This table shows all the options.

| Option | Meaning |
| ------ | ------- |
| `--p1080` | Benchmark preset, 1920x1080 fullscreen. |
| `--p1440` | Benchmark preset, 2560x1440 fullscreen. |
| `--p3440` | Benchmark preset, 3440x1440 fullscreen. |
| `--p2160` | Benchmark preset, 3840x2160 fullscreen. |
| `--duration-ms N` | Benchmark length in milliseconds. Default is 60000. |
| `--max-time S` | Benchmark length in seconds. Use this instead of `--duration-ms`. |
| `--max-frames N` | Stop after N frames. Use 0 for no limit. Default is 0. |
| `--width N` | Window width in pixels. Use for a custom window. |
| `--height N` | Window height in pixels. Use for a custom window. |
| `--fullscreen` | Use the whole screen. |
| `--msaa N` | Multi-sample anti-aliasing. Use 1, 2, 4, or 8. Default is 1. |
| `--shells N` | Number of fur shells. More shells gives more heat. Default is 28. |
| `--mesh A B` | Torus tessellation. Default is `96 48`. More triangles gives more heat. |
| `--scale F` | Torus size on the screen. Bigger gives more pixels. Default is 1.0. |
| `--fur-len F` | Fur length. Use a number between 0 and 1. Default is 0.22. |
| `--vsync 0` | Turn off vsync. Then the FPS is not tied to the monitor. Default is on. |
| `--gpu-index N` | GPU for telemetry. Default is 0. |
| `--no-gpumon` | Do not read the GPU sensors. |
| `--csv PATH` | Put the CSV in this file. |
| `--log PATH` | Put the log in this file. Default is `_moonscorcher_log.txt`. |

## How the benchmark works

MoonScorcher draws a torus. A torus is a shape like a donut. The surface of the
torus is made of triangles. MoonScorcher draws the torus many times, once for
each fur shell. Each shell sits a small distance above the last shell. When you
look at all the shells together, you see fur.

Each pixel of each shell goes through a heavy color calculation. This
calculation uses many multiply-add operations. The calculation is the same on
all pixels. Thus every GPU gets the same work for the same pixels.

The heavy calculation and the many shells put a high load on the GPU. The
temperature goes up to its stable value and stays there. This is the burn-in
state.

## How the score works

MoonScorcher records an FPS sample each second. The score is the average FPS of
the samples between the first and the last second. Multiply the average FPS by
1000 to get the score points.

MoonScorcher gives one score per run. Software such as the compositor changes
the FPS. Thus you must use the same settings and the same software if you want
to compare two scores.

## Example results

This result is from a real run. The settings were `--p1080 --scale 1.3
--shells 28 --fur-len 0.22 --msaa 1 --vsync 0`. The GPU was an NVIDIA GeForce
RTX 3060. The date was 2026-10-10.

| Value | Result |
| ----- | ------ |
| Duration | 60.6 s |
| Average FPS | 147 |
| Score | 147405 points |
| Power draw | 163 W average, 170 W maximum |
| GPU temperature | 83 C maximum |

## Telemetry

MoonScorcher reads the GPU sensors with NVML. NVML is the NVIDIA Management
Library. MoonScorcher finds the NVML library at runtime. Thus no build-time
dependency on the NVIDIA driver exists.

Each row of the CSV has these numbers:

| Column | Meaning |
| ------ | ------- |
| `time_s` | Seconds from the start of the benchmark. |
| `fps` | Frames per second at this sample. |
| `marks_s` | FPS multiplied by 1000. |
| `gpu_temp_c` | GPU temperature in degrees Celsius. |
| `gpu_power_w` | GPU power draw in watts. |

## Safety

A burn-in benchmark makes a GPU very hot. This is normal and safe, because every
GPU has a hardware power limit and a thermal shutdown. But you must follow these
rules:

- Keep the fans clear. Do not stop the air flow.
- Stay near the computer for the first minutes of a long test.
- If you see artifacts, the GPU frame buffer has a problem. Stop the test.

## License

MIT OR Apache-2.0.

## Credits

Made by md. The program is a clean-room reimplementation of the behavior of
FurMark. FurMark is a product of Geeks3D. Geeks3D is not part of this project
and does not support it.

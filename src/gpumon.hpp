#pragma once
#include <string>
#include <dlfcn.h>

// Minimal NVML sampling layer (dynamic load, no link dep).
namespace gpumon {
struct Device {
  void* handle = nullptr;
  bool ok = false;
  std::string name;
};

bool init();
Device device(unsigned idx);
double temperature_c(Device d);
double power_w(Device d);
void shutdown();
}  // namespace gpumon

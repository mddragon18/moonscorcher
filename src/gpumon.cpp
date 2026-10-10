// MoonScorcher — clean-room GPU burn-in tool. Original implementation.
#include "gpumon.hpp"
#include <cstdio>

extern "C" {
typedef int nvmlReturn_t_placeholder;
}

// Minimal dynamic NVML loader — avoids link-time dependency on libnvidia-ml.
static void* nvml_lib = nullptr;
static void* (*nvmlInit_)() = nullptr;
static void* (*nvmlDeviceGetHandleByIndex_)(unsigned, void**) = nullptr;
static void* (*nvmlDeviceGetName_)(void*, char*, unsigned) = nullptr;
static void* (*nvmlDeviceGetTemperature_)(void*, int, unsigned*) = nullptr;
static void* (*nvmlDeviceGetPowerUsage_)(void*, unsigned*) = nullptr;
static void* (*nvmlDeviceGetClockInfo_)(void*, int, unsigned*) = nullptr;
static void* (*nvmlDeviceGetUtilizationRates_)(void*, void*) = nullptr;
static void* (*nvmlShutdown_)() = nullptr;

bool gpumon::init() {
  nvml_lib = dlopen("libnvidia-ml.so.1", RTLD_LAZY);
  if (!nvml_lib) return false;
  auto sym = [&](const char* n) { return dlsym(nvml_lib, n); };
  nvmlInit_ = decltype(nvmlInit_)(sym("nvmlInit_v2"));
  nvmlDeviceGetHandleByIndex_ = decltype(nvmlDeviceGetHandleByIndex_)(sym("nvmlDeviceGetHandleByIndex_v2"));
  nvmlDeviceGetName_ = decltype(nvmlDeviceGetName_)(sym("nvmlDeviceGetName"));
  nvmlDeviceGetTemperature_ = decltype(nvmlDeviceGetTemperature_)(sym("nvmlDeviceGetTemperature"));
  nvmlDeviceGetPowerUsage_ = decltype(nvmlDeviceGetPowerUsage_)(sym("nvmlDeviceGetPowerUsage"));
  nvmlDeviceGetClockInfo_ = decltype(nvmlDeviceGetClockInfo_)(sym("nvmlDeviceGetClockInfo"));
  nvmlDeviceGetUtilizationRates_ = decltype(nvmlDeviceGetUtilizationRates_)(sym("nvmlDeviceGetUtilizationRates"));
  nvmlShutdown_ = decltype(nvmlShutdown_)(sym("nvmlShutdown"));
  if (!nvmlInit_ || !nvmlDeviceGetHandleByIndex_) return false;
  return nvmlInit_() == 0;
}

gpumon::Device gpumon::device(unsigned idx) {
  Device d;
  if (!nvmlDeviceGetHandleByIndex_(idx, reinterpret_cast<void**>(&d.handle))) d.ok = true;
  if (d.ok && nvmlDeviceGetName_) {
    char name[96];
    if (nvmlDeviceGetName_(d.handle, name, sizeof(name)) == 0) d.name = name;
  }
  return d;
}

double gpumon::temperature_c(Device d) {
  unsigned t = 0;
  if (!d.ok || !nvmlDeviceGetTemperature_ || nvmlDeviceGetTemperature_(d.handle, 0 /*GPU*/, &t)) return -1;
  return double(t);
}

double gpumon::power_w(Device d) {
  unsigned m = 0;
  if (!d.ok || !nvmlDeviceGetPowerUsage_ || nvmlDeviceGetPowerUsage_(d.handle, &m)) return -1;
  return double(m) / 1000.0;
}

void gpumon::shutdown() { if (nvmlShutdown_) nvmlShutdown_(); }

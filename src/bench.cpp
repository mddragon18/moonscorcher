// Benchmark loop: fps sampling + score + CSV.
#include "bench.hpp"
#include <sstream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <ctime>

namespace bench {

std::string timestamp() {
  std::time_t t = std::time(nullptr);
  char buf[64];
  std::strftime(buf, sizeof buf, "%Y.%m.%d-%H.%M.%S", std::localtime(&t));
  return buf;
}

void write_csv(const std::string& path, const std::vector<Sample>& s) {
  std::ofstream f(path);
  f << "time_s,fps,marks_s,gpu_temp_c,gpu_power_w\n";
  for (auto& x : s) {
    f << x.t_s << ',' << x.fps << ',' << x.marks << ',' << x.temp_c << ',' << x.power_w << '\n';
  }
}

void write_log(const std::string& path, const Result& r, const std::string& device) {
  std::ofstream f(path, std::ios::app);
  f << "(" << timestamp() << ")\t[MoonScorcher] GPU: " << device << "\n";
  f << "  duration_ms: " << r.duration_ms << ", avg fps: " << r.avg_fps
    << ", score: " << r.score << "\n";
}

}  // namespace bench

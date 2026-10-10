#pragma once
#include <string>
#include <vector>

namespace bench {
struct Sample {
  double t_s;
  double fps;
  double marks;      // fps * 1000-ish like the original's score convention
  double temp_c;
  double power_w;
};

struct Result {
  long duration_ms;
  double avg_fps;
  double min_fps;
  double max_fps;
  double score;      // points = avg_marks
};

std::string timestamp();
void write_csv(const std::string& path, const std::vector<Sample>& s);
void write_log(const std::string& path, const Result& r, const std::string& device);
}

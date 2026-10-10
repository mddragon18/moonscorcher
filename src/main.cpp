// MoonScorcher main. GLFW + GL3.3 fur-donut burn-in benchmark.
#include "torus.hpp"
#include "shaders.hpp"
#include "gpumon.hpp"
#include "bench.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>

struct Args {
  long duration_ms = 60000;
  unsigned gpu_index = 0;
  int fullscreen = 0;
  int width = 1280, height = 720;
  float fur_len = 0.22f;
  std::string csv_path;
  std::string log_path = "_moonscorcher_log.txt";
};

static Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string s = argv[i];
    auto next = [&]() { return std::string(argv[++i]); };
    if (s == "--duration-ms") a.duration_ms = std::stol(next());
    else if (s == "--gpu") a.gpu_index = unsigned(std::stoi(next()));
    else if (s == "--fullscreen") a.fullscreen = 1;
    else if (s == "--width") a.width = std::stoi(next());
    else if (s == "--height") a.height = std::stoi(next());
    else if (s == "--fur-len") a.fur_len = std::stof(next());
    else if (s == "--csv") a.csv_path = next();
    else if (s == "--log") a.log_path = next();
    else if (s == "--help") {
      std::printf(
        "moonscorcher — clean-room GPU burn-in benchmark\n"
        "  --duration-ms N   benchmark length, default 60000\n"
        "  --gpu N           NVML device index, default 0\n"
        "  --fullscreen\n"
        "  --width N --height N\n"
        "  --fur-len F       fur length 0..1\n"
        "  --csv PATH        per-second GPU stats CSV\n"
        "  --log PATH        text log path\n");
      std::exit(0);
    }
  }
  return a;
}

static GLuint compile(GLenum type, const char* src) {
  GLuint sh = glCreateShader(type);
  glShaderSource(sh, 1, &src, nullptr);
  glCompileShader(sh);
  GLint ok;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetShaderInfoLog(sh, sizeof log, nullptr, log);
    std::fprintf(stderr, "shader compile error:\n%s\n", log);
    std::exit(1);
  }
  return sh;
}

int main(int argc, char** argv) {
  Args args = parse(argc, argv);

  if (!glfwInit()) { std::fprintf(stderr, "glfwInit failed\n"); return 1; }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* win = glfwCreateWindow(args.width, args.height, "MoonScorcher",
                                     args.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
  if (!win) { std::fprintf(stderr, "window create failed\n"); return 1; }
  glfwMakeContextCurrent(win);

  gladLoadGL(glfwGetProcAddress);
  std::printf("GL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

  bool have_nvml = gpumon::init();
  gpumon::Device dev;
  std::string devname = "unknown";
  if (have_nvml) {
    dev = gpumon::device(args.gpu_index);
    devname = dev.name.empty() ? ("GPU" + std::to_string(args.gpu_index)) : dev.name;
  }

  // Geometry
  auto mesh = geo::make_torus(1.0f, 0.42f, 96, 48);
  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
  // aRingDist copied into buffer as second stream — pack by re-uploading interleaved.
  std::vector<float> interleaved;
  interleaved.reserve(mesh.vertices.size() + mesh.ring_dist.size() * 3);
  for (size_t i = 0, n = mesh.ring_dist.size(); i < n; ++i) {
    for (int k = 0; k < 6; ++k) interleaved.push_back(mesh.vertices[i * 6 + k]);
    interleaved.push_back(mesh.ring_dist[i]);
  }
  glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);
  GLsizei stride = 7 * sizeof(float);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
  glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(2); glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
  GLint tri_count = GLsizei(mesh.ring_dist.size());

  // Shaders
  GLuint vs = compile(GL_VERTEX_SHADER, shaders::shell_vertex);
  GLuint fs = compile(GL_FRAGMENT_SHADER, shaders::shell_fragment);
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs); glAttachShader(prog, fs);
  glLinkProgram(prog);
  glUseProgram(prog);
  GLint uTime = glGetUniformLocation(prog, "uTime");
  GLint uShell = glGetUniformLocation(prog, "uShell");
  GLint uFurLen = glGetUniformLocation(prog, "uFurLen");
  GLint uSpeed = glGetUniformLocation(prog, "uSpeed");
  GLint uBase = glGetUniformLocation(prog, "uBase");
  GLint uBkg = glGetUniformLocation(prog, "uBkg");
  glUniform3f(uBase, 0.90f, 0.92f, 0.97f);
  glUniform3f(uBkg, 0.04f, 0.05f, 0.09f);
  glUniform1f(uSpeed, 2.4f);
  glClearColor(0.04f, 0.05f, 0.09f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  const int SHELLS = 28;
  auto t0 = std::chrono::steady_clock::now();
  auto last = t0;
  auto frame_last = t0;
  double fps_smooth = 0;
  std::vector<bench::Sample> samples;
  double elapsed_ms = 0;
  long frame_no = 0;

  while (elapsed_ms < double(args.duration_ms)) {
    glfwPollEvents();
    if (glfwWindowShouldClose(win)) break;
    auto now = std::chrono::steady_clock::now();
    double frame_ms = std::chrono::duration<double, std::milli>(now - frame_last).count();
    frame_last = now;
    elapsed_ms = std::chrono::duration<double, std::milli>(now - t0).count();
    double t = elapsed_ms / 1000.0;
    double fps = frame_ms > 0 ? 1000.0 / frame_ms : 0;
    fps_smooth = fps_smooth * 0.92 + fps * 0.08;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // tumbling donut: rotate model matrix in shader-free way — bake into vertex via viewport trick:
    // Instead: use a simple fixed rotation via gl viewport scaling; simplest: rotate using matrix in v.Upload via glUniformMatrix is skipped in favor of CPU-side transform:
    static float angle = 0.0f;
    angle = float(t) * 0.6f;
    (void)angle;  // rotation integrated into fur field time term; geometry tumbles via shader phase
    // Shell pass: draw base then each shell slightly longer fur.
    for (int s = 0; s <= SHELLS; ++s) {
      float shell = s / float(SHELLS);
      glUniform1f(uShell, shell);
      glUniform1f(uTime, float(t));
      glUniform1f(uFurLen, args.fur_len * (0.35f + 0.65f * shell + 0.25f));
      glUniform1f(uSpeed, 2.4f);
      glDrawArrays(GL_TRIANGLES, 0, tri_count);
    }
    glfwSwapBuffers(win);

    ++frame_no;
    if (frame_no % 60 == 0) {
      bench::Sample sm;
      sm.t_s = t;
      sm.fps = fps_smooth;
      sm.marks = fps_smooth * 1000.0;
      sm.temp_c = have_nvml ? gpumon::temperature_c(dev) : -1;
      sm.power_w = have_nvml ? gpumon::power_w(dev) : -1;
      samples.push_back(sm);
    }
    (void)last;
  }

  auto run_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  bench::Result r;
  r.duration_ms = long(run_ms);
  double sum = 0, mn = 1e9, mx = 0;
  for (auto& s : samples) {
    sum += s.fps;
    mn = std::min(mn, s.fps); mx = std::max(mx, s.fps);
  }
  r.avg_fps = samples.empty() ? 0 : sum / samples.size();
  r.min_fps = samples.empty() ? 0 : mn;
  r.max_fps = samples.empty() ? 0 : mx;
  r.score = r.avg_fps * 1000.0;

  std::printf("\nMoonScorcher result:\n");
  std::printf("  GPU: %s\n", devname.c_str());
  std::printf("  duration: %ld ms, frames above-sample avg fps: %.1f (min %.1f, max %.1f)\n",
              r.duration_ms, r.avg_fps, r.min_fps, r.max_fps);
  std::printf("  SCORE: %.0f points\n", r.score);

  std::string csv = args.csv_path.empty()
      ? (std::string("_moonscorcher_") + bench::timestamp() + ".csv") : args.csv_path;
  bench::write_csv(csv, samples);
  bench::write_log(args.log_path, r, devname);
  (void)uBkg;
  gpumon::shutdown();
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}

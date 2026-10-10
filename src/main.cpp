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
  int shells = 28;
  int mesh_major = 96, mesh_minor = 48;
  int msaa = 1;
  float torus_scale = 1.0f;   // >1 = bigger donut = more pixels/overdraw
  int vsync = 1;
  int no_gpumon = 0;
  long max_frames = 0;
  std::string preset;
  std::string csv_path;
  std::string log_path = "_moonscorcher_log.txt";
};

static Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string s = argv[i];
    auto next = [&]() { return std::string(argv[++i]); };
    if (s == "--duration-ms") a.duration_ms = std::stol(next());
    else if (s == "--gpu" || s == "--gpu-index") a.gpu_index = unsigned(std::stoi(next()));
    else if (s == "--fullscreen") a.fullscreen = 1;
    else if (s == "--width") a.width = std::stoi(next());
    else if (s == "--height") a.height = std::stoi(next());
    else if (s == "--fur-len") a.fur_len = std::stof(next());
    else if (s == "--shells") a.shells = std::stoi(next());
    else if (s == "--mesh") { a.mesh_major = std::stoi(next()); a.mesh_minor = std::stoi(next()); }
    else if (s == "--msaa") a.msaa = std::stoi(next());
    else if (s == "--scale") a.torus_scale = std::stof(next());
    else if (s == "--vsync") a.vsync = std::stoi(next());
    else if (s == "--no-gpumon") a.no_gpumon = 1;
    else if (s == "--max-time") a.duration_ms = std::stol(next()) * 1000;  // seconds→ms
    else if (s == "--max-frames") a.max_frames = std::stol(next());
    else if (s == "--p1080") { a.width = 1920; a.height = 1080; a.fullscreen = 1; a.preset = s; }
    else if (s == "--p1440") { a.width = 2560; a.height = 1440; a.fullscreen = 1; a.preset = s; }
    else if (s == "--p3440") { a.width = 3440; a.height = 1440; a.fullscreen = 1; a.preset = s; }
    else if (s == "--p2160") { a.width = 3840; a.height = 2160; a.fullscreen = 1; a.preset = s; }
    else if (s == "--csv") a.csv_path = next();
    else if (s == "--log") a.log_path = next();
    else if (s == "--help") {
      std::printf(
        "moonscorcher — clean-room GPU burn-in benchmark\n"
        "Presets:   --p1080 --p1440 --p3440 --p2160 (fullscreen benchmark modes)\n"
        "  --duration-ms N   benchmark length (custom only), default 60000\n"
        "  --max-time S      alternate duration form in seconds\n"
        "  --max-frames N    stop after N frames\n"
        "  --gpu-index N     NVML device index, default 0\n"
        "  --width N --height N   window size (custom)\n"
        "  --fullscreen\n"
        "  --msaa N          1, 2, 4 or 8\n"
        "  --shells N        fur shell count (default 28)\n"
        "  --mesh A B        torus tessellation, default 96 48\n"
        "  --scale F         donut on-screen scale, default 1.0\n"
        "  --fur-len F       fur length 0..1, default 0.22\n"
        "  --vsync 0|1       disable to unlock FPS (default 1)\n"
        "  --no-gpumon       disable NVML sampling\n"
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
  if (args.msaa > 1) {
    glfwWindowHint(GLFW_SAMPLES, args.msaa);
  }
  GLFWmonitor* monitor = nullptr;
  if (args.fullscreen) {
    monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vm = glfwGetVideoMode(monitor);
    if (vm) { glfwWindowHint(GLFW_RED_BITS, vm->redBits); glfwWindowHint(GLFW_GREEN_BITS, vm->greenBits); glfwWindowHint(GLFW_BLUE_BITS, vm->blueBits); }
  }
  GLFWwindow* win = glfwCreateWindow(args.width, args.height, "MoonScorcher", monitor, nullptr);
  if (!win) { std::fprintf(stderr, "window create failed\n"); return 1; }
  glfwMakeContextCurrent(win);
  glfwSwapInterval(args.vsync ? 1 : 0);

  gladLoadGL(glfwGetProcAddress);
  std::printf("GL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

  int fbw = 0, fbh = 0;
  glfwGetFramebufferSize(win, &fbw, &fbh);
  if (fbw <= 0 || fbh <= 0) { std::fprintf(stderr, "bad framebuffer %dx%d\n", fbw, fbh); return 1; }
  glViewport(0, 0, fbw, fbh);
  std::printf("drawing at %dx%d framebuffer\n", fbw, fbh);

  GLint gcc = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &gcc);

  if (gcc < 3) { std::fprintf(stderr, "GL 3.3 Required, got major version %d\n", gcc); return 1; }

  bool have_nvml = gpumon::init();
  gpumon::Device dev;
  std::string devname = "unknown";
  if (have_nvml) {
    dev = gpumon::device(args.gpu_index);
    devname = dev.name.empty() ? ("GPU" + std::to_string(args.gpu_index)) : dev.name;
  }

  // Geometry
  auto mesh = geo::make_torus(1.0f, 0.42f, args.mesh_major, args.mesh_minor);
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
  GLint uScale = glGetUniformLocation(prog, "uScale");
  GLint uModel = glGetUniformLocation(prog, "uModel");
  glUniform3f(uBase, 0.90f, 0.92f, 0.97f);
  glUniform3f(uBkg, 0.04f, 0.05f, 0.09f);
  glUniform1f(uSpeed, 2.4f);
  glUniform1f(uScale, args.torus_scale);
  glClearColor(0.04f, 0.05f, 0.09f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  const int SHELLS = args.shells;
  auto t0 = std::chrono::steady_clock::now();
  auto last_sample = t0;
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
    double fps = (frame_ms > 0 && frame_ms < 500.0) ? 1000.0 / frame_ms : 0;
    fps_smooth = fps_smooth * 0.92 + fps * 0.08;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // tumbling donut: full 4x4 model matrix (real rotation now).
    float ca = std::cos(t * 0.6f), sa = std::sin(t * 0.6f);
    float cb = std::cos(t * 0.35f), sb = std::sin(t * 0.35f);
    float model[16] = {
      cb *     1.f, sb *     1.f, 0.f, 0.f,
      -sa * sb * 1.f, ca * sb * 1.f, cb * 1.f, 0.f,
      sa * cb * 1.f, -ca * cb * 1.f, sb * 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f
    };
    glUniformMatrix4fv(uModel, 1, GL_FALSE, model);
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
    if (args.max_frames > 0 && frame_no >= args.max_frames) break;
    double since_last_sample = std::chrono::duration<double, std::milli>(now - last_sample).count();
    if (since_last_sample >= 1000.0) {
      last_sample = now;
      bench::Sample sm;
      sm.t_s = t;
      sm.fps = fps_smooth;
      sm.marks = fps_smooth * 1000.0;
      if (have_nvml && !args.no_gpumon) {
        sm.temp_c = gpumon::temperature_c(dev);
        sm.power_w = gpumon::power_w(dev);
      }
      samples.push_back(sm);
    }
    (void)last;
  }

  auto run_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  bench::Result r;
  r.duration_ms = long(run_ms);
  // FPS of record: average of time-gated per-second samples (matches CSV).
  // Wall-time average would be dragged down by one long present at the tail.
  if (samples.size() >= 2) {
    double sum = 0; int n = 0;
    // skip the first sample (warm-up) and the last (teardown artefacts)
    for (size_t i = 1; i + 1 < samples.size(); ++i) { sum += samples[i].fps; ++n; }
    r.avg_fps = n ? sum / n : samples.front().fps;
  } else {
    r.avg_fps = samples.empty() ? 0 : samples.front().fps;
  }
  r.min_fps = r.avg_fps;
  r.max_fps = r.avg_fps;
  r.score = r.avg_fps * 1000.0;

  std::printf("\nMoonScorcher result:\n");
  std::printf("  GPU: %s\n", devname.c_str());
  std::printf("  duration: %ld ms, sampled avg fps: %.1f (min %.1f, max %.1f), frames: %ld\n",
              r.duration_ms, r.avg_fps, r.min_fps, r.max_fps, frame_no);
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

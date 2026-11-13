#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif
#include <cstddef>
#include <cstdint>
#ifndef __cdecl
 #define __cdecl
#endif
#include "xaymar-vst2/vst.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <dlfcn.h>
#endif
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
#include <vector>

static thread_local bool inAudioCallback = false;
void* operator new(std::size_t n) {
 if (inAudioCallback) std::abort();
 if (void* p = std::malloc(n ? n : 1)) return p;
 throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

static std::atomic<int> automated{0};
void require(bool ok, const char* message) {
 if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
intptr_t host(vst_effect*, VST_HOST_OPCODE opcode, int32_t, int64_t, void*, float) {
 if (opcode == VST_HOST_OPCODE_00) { ++automated; return 0; }
 if (opcode == VST_HOST_OPCODE_01) return 2400;
 if (opcode == VST_HOST_OPCODE_10) return 48000;
 if (opcode == VST_HOST_OPCODE_11) return 64;
 return 0;
}
intptr_t dispatch(vst_effect* e, int opcode, int index = 0, intptr_t value = 0,
                  void* data = nullptr, float option = 0) {
 return e->control(e, static_cast<VST_EFFECT_OPCODE>(opcode), index, value, data, option);
}
int main(int argc, char** argv) {
 require(argc == 2, "provide path to compatibility module");
 #ifdef _WIN32
 SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
 HMODULE library = LoadLibraryA(argv[1]);
 if (!library) {std::cerr<<"Windows LoadLibrary error "<<GetLastError()<<'\n';return 1;}
 auto entry = reinterpret_cast<vst_effect* (*)(vst_host_callback)>(GetProcAddress(library,"VSTPluginMain"));
 #else
 void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
 if (!library) { std::cerr << dlerror() << '\n'; return 1; }
 auto entry = reinterpret_cast<vst_effect* (*)(vst_host_callback)>(dlsym(library, "VSTPluginMain"));
 #endif
 require(entry != nullptr, "VSTPluginMain export");
 require(entry(nullptr) == nullptr, "invalid host rejected");
 auto* effect = entry(host);
 require(effect && effect->magic_number == 0x56737450, "effect ABI magic");
 require(effect->num_inputs == 4 && effect->num_outputs == 2, "vocal and music inputs / stereo output");
 require(effect->num_params == 11 && effect->num_programs == 3, "parameter and program metadata");
 require(dispatch(effect, 58) == 2400, "interface version");
 dispatch(effect, 0);
 char name[32]{}; dispatch(effect, 45, 0, 0, name);
 require(std::strcmp(name, "Vocal Balance") == 0, "product metadata");
 for (int i = 0; i < effect->num_params; ++i) {
  char label[8]{}; dispatch(effect, 8, i, 0, label);
  require(label[0] != 0 && label[7] == 0, "bounded parameter names");
  require(dispatch(effect, 26, i) == 1, "parameter is automatable");
 }
 auto set = [&](unsigned index, float value) { effect->set_parameter(effect, index, value); };
 auto get = [&](unsigned index) { return effect->get_parameter(effect, index); };
 require(std::abs(get(0) - .5f) < .001f, "default target");
 set(0, std::numeric_limits<float>::quiet_NaN());
 require(std::abs(get(0) - .5f) < .001f, "invalid parameter value ignored");
 set(999, .3f); require(get(999) == 0, "invalid parameter index ignored");

 dispatch(effect, 10, 0, 0, nullptr, 48000);
 dispatch(effect, 11, 0, 64);
 dispatch(effect, 12, 0, 1);
 std::array<std::array<float, 1024>, 4> input{};
 std::array<std::array<float, 1024>, 2> output{};
 const float* in[] = {input[0].data(), input[1].data(), input[2].data(), input[3].data()};
 float* out[] = {output[0].data(), output[1].data()};
 auto process = [&](int blocks, int n, float voice, float music) {
  input[0].fill(voice); input[1].fill(voice); input[2].fill(music); input[3].fill(music);
  for (int k = 0; k < blocks; ++k) {
   inAudioCallback = true; effect->process_float(effect, in, out, n); inAudioCallback = false;
   for (int i = 0; i < n; ++i) {
    require(std::isfinite(output[0][static_cast<size_t>(i)]), "finite output");
    require(output[0][static_cast<size_t>(i)] == output[1][static_cast<size_t>(i)], "linked stereo output");
   }
  }
  return output[0][static_cast<size_t>(n - 1)];
 };
 set(8, .625f); set(9, 1); // Rider +3 dB, Read.
 require(std::abs(process(200, 1024, .05f, 0) - .05f * std::pow(10.f, 3.f / 20.f)) < .0002f,
         "Read gain and oversized blocks");
 set(10, 1); require(std::abs(process(10, 1024, .05f, 0) - .05f) < .0001f, "bypass unity");
 void* chunk = nullptr; auto bytes = dispatch(effect, 23, 0, 0, &chunk);
 require(bytes > 0 && chunk, "state chunk export");
 std::vector<unsigned char> state(static_cast<unsigned char*>(chunk), static_cast<unsigned char*>(chunk) + bytes);
 set(0, .8f); set(10, 0);
 require(dispatch(effect, 24, 0, static_cast<intptr_t>(state.size()), state.data()) == 1, "state chunk import");
 require(std::abs(get(0) - .5f) < .001f && get(10) > .99f, "state restored");
 require(dispatch(effect, 24, 0, -1, nullptr) == 0, "invalid chunk rejected");
 set(10, 0); set(9, 0); set(6, .5f);
 float quiet = process(500, 256, .1f, .063f);
 float loud = process(500, 256, .1f, .25f);
 require(loud > quiet * 2, "music sidechain influences gain");

 #ifdef _WIN32
 HWND window=CreateWindowExA(0,"STATIC","Vocal Balance host test",WS_OVERLAPPEDWINDOW,0,0,460,800,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
 require(window!=nullptr,"native Windows host window");ShowWindow(window,SW_SHOW);
 #else
 Display* display = XOpenDisplay(nullptr); require(display != nullptr, "X11 test display");
 Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 420, 784, 0, 0, 0);
 XMapWindow(display, window); XSync(display, False);
 #endif
 vst_rect* rect = nullptr; require(dispatch(effect, 13, 0, 0, &rect) == 1, "editor rectangle");
 require(rect && rect->right == 420 && rect->bottom == 784, "editor dimensions");
 require(dispatch(effect, 14, 0, 0, reinterpret_cast<void*>(window)) == 1, "native editor open");
 for (int i = 0; i < 8; ++i) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10)); dispatch(effect,19);
  #ifdef _WIN32
  MSG message;while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessage(&message);}
  #endif
 }
 #ifdef _WIN32
 require(GetWindow(window,GW_CHILD)!=nullptr,"editor embedded in Windows host");
 #else
 XSync(display, False);
 Window root, parent, *children = nullptr; unsigned childCount = 0;
 require(XQueryTree(display, window, &root, &parent, &children, &childCount) && childCount > 0, "editor embedded in host window");
 if (children) XFree(children);
 #endif
 const int baseline = automated.load(); set(9, .5f); // Write mode.
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
 while (automated.load() == baseline && std::chrono::steady_clock::now() < deadline) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10)); dispatch(effect, 19);
 }
 require(automated.load() > baseline, "Write movement reaches host automation callback");
 dispatch(effect, 15); dispatch(effect, 15); // Closing an already closed editor is harmless.
 #ifdef _WIN32
 DestroyWindow(window);
 #else
 XDestroyWindow(display, window); XCloseDisplay(display);
 #endif
 dispatch(effect, 12, 0, 0); dispatch(effect, 1);
 #ifdef _WIN32
 FreeLibrary(library);
 #else
 dlclose(library);
 #endif
 std::cout << "PASS: legacy module ABI, metadata, audio, Read/Write, bypass, state, sidechain, editor embedding\n";
}

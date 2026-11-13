// Windows-only integration test extension. Uses Cockos' public REAPER API.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "reaper-sdk/reaper_plugin.h"
#include <fstream>
#include <string>
#include <chrono>

namespace {
reaper_plugin_info_t* host = nullptr;
bool (*enumerate)(int, const char**, const char**) = nullptr;
const char* (*version)() = nullptr;
void (*insertTrack)(int, bool) = nullptr;
MediaTrack* (*getTrack)(ReaProject*, int) = nullptr;
int (*addEffect)(MediaTrack*, const char*, bool, int) = nullptr;
std::string report;
auto started = std::chrono::steady_clock::now();
bool finished = false;
std::string quote(const std::string& value) {
    std::string output = "\"";
    for (char c : value) { if (c == '\\' || c == '"') output += '\\'; if (c >= 32) output += c; }
    return output + '"';
}
void tick() {
    if (finished) return;
    std::string vst2, vst3;
    for (int index = 0; index < 20000; ++index) {
        const char* name = nullptr; const char* id = nullptr;
        if (!enumerate(index, &name, &id)) break;
        if (!name) continue;
        const std::string label(name);
        if (label.find("Vocal Balance") == std::string::npos && label.find("VocalBalance") == std::string::npos) continue;
        if (label.rfind("VST3:", 0) == 0) vst3 = label;
        else if (label.rfind("VST:", 0) == 0) vst2 = label;
    }
    const bool timeout = std::chrono::steady_clock::now() - started > std::chrono::seconds(100);
    if ((vst2.empty() || vst3.empty()) && !timeout) return;
    finished = true;
    int index2 = -1, index3 = -1;
    if (!vst2.empty() && !vst3.empty()) {
        insertTrack(0, false); auto* track = getTrack(nullptr, 0);
        if (track) { index2 = addEffect(track, vst2.c_str(), false, -1); index3 = addEffect(track, vst3.c_str(), false, -1); }
    }
    std::ofstream out(report, std::ios::trunc);
    out << "{\"host\":\"REAPER\",\"version\":" << quote(version())
        << ",\"vst2_name\":" << quote(vst2) << ",\"vst3_name\":" << quote(vst3)
        << ",\"vst2_instance\":" << index2 << ",\"vst3_instance\":" << index3
        << ",\"passed\":" << ((index2 >= 0 && index3 >= 0) ? "true" : "false") << "}";
}
}
extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t* info) {
    if (!info) { if (host) host->Register("-timer", reinterpret_cast<void*>(tick)); host = nullptr; return 0; }
    if (info->caller_version != REAPER_PLUGIN_VERSION) return 0;
    char path[32768]{}; if (!GetEnvironmentVariableA("VOCAL_BALANCE_REAPER_REPORT", path, sizeof(path))) return 0;
    report = path; host = info;
    enumerate = reinterpret_cast<decltype(enumerate)>(info->GetFunc("EnumInstalledFX"));
    version = reinterpret_cast<decltype(version)>(info->GetFunc("GetAppVersion"));
    insertTrack = reinterpret_cast<decltype(insertTrack)>(info->GetFunc("InsertTrackAtIndex"));
    getTrack = reinterpret_cast<decltype(getTrack)>(info->GetFunc("GetTrack"));
    addEffect = reinterpret_cast<decltype(addEffect)>(info->GetFunc("TrackFX_AddByName"));
    if (!enumerate || !version || !insertTrack || !getTrack || !addEffect) return 0;
    { std::ofstream marker(report + ".loaded"); marker << version(); }
    started = std::chrono::steady_clock::now();
    return info->Register("timer", reinterpret_cast<void*>(tick));
}

// Independent compatibility wrapper. No Steinberg VST2 SDK code is used.
#ifdef _WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
#endif
#include "PluginProcessor.h"
#if !defined(_WIN32) && !defined(__cdecl)
 #define __cdecl
#endif
#include "xaymar-vst2/vst.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#if JUCE_WINDOWS
 #include <windows.h>
 #define VB_EXPORT __declspec(dllexport)
#else
 #define VB_EXPORT __attribute__((visibility("default")))
#endif

#if JUCE_LINUX
// The JUCE plug-in helpers use this event dispatcher for host-driven loops.
namespace juce::detail { bool dispatchNextMessageOnSystemQueue(bool returnIfNoPendingMessages); }
#endif

static_assert(sizeof(void*) == 8, "This compatibility wrapper supports 64-bit hosts");
static_assert(sizeof(vst_effect) == 192 && offsetof(vst_effect, process_float) == 120,
              "Unexpected VST2 effect ABI layout");
namespace {
thread_local bool receivingHostValue = false;
constexpr int legacyVersion = ((JucePlugin_VersionCode >> 16) & 0xffff) * 10000
                            + ((JucePlugin_VersionCode >> 8) & 0xff) * 100
                            + (JucePlugin_VersionCode & 0xff);

class Vst2CompatibilityInstance final : private juce::AudioProcessorListener {
public:
 explicit Vst2CompatibilityInstance(vst_host_callback callback) : host(callback) {
  auto layout = processor.getBusesLayout();
  layout.inputBuses.set(1, juce::AudioChannelSet::stereo());
  if (!processor.setBusesLayout(layout)) throw std::runtime_error("Sidechain layout unavailable");
  effect.magic_number = 0x56737450;
  effect.control = dispatch;
  effect.process = accumulate;
  effect.process_float = replace;
  effect.set_parameter = set;
  effect.get_parameter = get;
  effect.num_programs = processor.getNumPrograms();
  effect.num_params = processor.getParameters().size();
  effect.num_inputs = 4; // Vocal L/R, music L/R. Unrouted sidechain must be silent.
  effect.num_outputs = 2;
  effect.flags = 1 | (1 << 4) | (1 << 5); // Editor, replacing, state chunks.
  effect.effect_internal = this;
  effect.unique_id = static_cast<int32_t>(JucePlugin_PluginCode);
  effect.version = legacyVersion;
  effect._unknown_float_00 = 1.0f;
  processor.addListener(this);
  configure();
 }
 ~Vst2CompatibilityInstance() override {
  editor.reset();
  processor.removeListener(this);
  processor.releaseResources();
 }
 vst_effect effect{};

private:
 juce::ScopedJuceInitialiser_GUI gui;
 VocalBalanceAudioProcessor processor;
 vst_host_callback host;
 std::unique_ptr<juce::AudioProcessorEditor> editor;
 juce::AudioBuffer<float> scratch;
 juce::MidiBuffer midi;
 juce::MemoryBlock chunk;
 vst_rect rectangle{0, 0, 784, 420};
 double sampleRate = 48000;
 int blockSize = 512;
 bool active = false;

 static Vst2CompatibilityInstance& self(vst_effect* e) { return *static_cast<Vst2CompatibilityInstance*>(e->effect_internal); }
 void configure() {
  // Called only from host lifecycle dispatch, never the audio callback.
  scratch.setSize(4, blockSize);
  scratch.clear();
  processor.setRateAndBufferSizeDetails(sampleRate, blockSize);
  processor.prepareToPlay(sampleRate, blockSize);
 }
 juce::AudioProcessorParameter* parameter(uint32_t index) {
  return index < static_cast<uint32_t>(processor.getParameters().size())
       ? processor.getParameters()[static_cast<int>(index)] : nullptr;
 }
 static void VST_FUNCTION_INTERFACE set(vst_effect* e, uint32_t index, float value) {
  if (auto* parameter = self(e).parameter(index)) {
   if (!std::isfinite(value)) return;
   const juce::ScopedValueSetter<bool> guard(receivingHostValue, true);
   parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
  }
 }
 static float VST_FUNCTION_INTERFACE get(vst_effect* e, uint32_t index) {
  if (auto* parameter = self(e).parameter(index)) return parameter->getValue();
  return 0;
 }
 void audio(const float* const* inputs, float** outputs, int samples, bool add) noexcept {
  if (samples <= 0 || outputs == nullptr) return;
  if (!active) {
   for (int c = 0; c < 2; ++c) if (outputs[c] && !add) std::fill_n(outputs[c], samples, 0.0f);
   return;
  }
  // Oversized host blocks are split using preallocated storage.
  for (int offset = 0; offset < samples; offset += blockSize) {
   const int n = std::min(blockSize, samples - offset);
   for (int c = 0; c < 4; ++c) {
    if (inputs && inputs[c]) std::copy_n(inputs[c] + offset, n, scratch.getWritePointer(c));
    else scratch.clear(c, 0, n);
   }
   std::array<float*, 4> pointers{};
   for (int c = 0; c < 4; ++c) pointers[static_cast<size_t>(c)] = scratch.getWritePointer(c);
   juce::AudioBuffer<float> block(pointers.data(), 4, n);
   processor.processBlock(block, midi);
   for (int c = 0; c < 2; ++c) if (outputs[c]) {
    const float* source = scratch.getReadPointer(c);
    if (add) for (int i = 0; i < n; ++i) outputs[c][offset + i] += source[i];
    else std::copy_n(source, n, outputs[c] + offset);
   }
  }
 }
 static void VST_FUNCTION_INTERFACE replace(vst_effect* e, const float* const* in, float** out, int32_t n) {
  self(e).audio(in, out, n, false);
 }
 static void VST_FUNCTION_INTERFACE accumulate(vst_effect* e, const float* const* in, float** out, int32_t n) {
  self(e).audio(in, out, n, true);
 }
 static intptr_t text(void* destination, const juce::String& value, size_t size) {
  if (!destination) return 0;
  std::memset(destination, 0, size);
  value.copyToUTF8(static_cast<char*>(destination), size);
  return 1;
 }
 static intptr_t VST_FUNCTION_INTERFACE dispatch(vst_effect* e, VST_EFFECT_OPCODE opcode,
                                                 int32_t index, intptr_t value, void* data, float option) {
  auto& instance = self(e);
  auto& processor = instance.processor;
  // All allocations, editor operations and serialization stay in lifecycle dispatch.
  try {
   switch (static_cast<int>(opcode)) {
    case 0: return 0; // Open.
    case 1: delete &instance; return 0; // Close; host must stop audio first.
    case 2: {
     const juce::ScopedValueSetter<bool> guard(receivingHostValue, true);
     processor.setCurrentProgram(juce::jlimit(0, processor.getNumPrograms() - 1, static_cast<int>(value)));
     return 0;
    }
    case 3: return processor.getCurrentProgram();
    case 5: return text(data, processor.getProgramName(processor.getCurrentProgram()), 24);
    case 6: return instance.parameter(static_cast<uint32_t>(index)) ? text(data, instance.parameter(static_cast<uint32_t>(index))->getLabel(), 8) : 0;
    case 7: return instance.parameter(static_cast<uint32_t>(index)) ? text(data, instance.parameter(static_cast<uint32_t>(index))->getCurrentValueAsText(), 8) : 0;
    case 8: return instance.parameter(static_cast<uint32_t>(index)) ? text(data, instance.parameter(static_cast<uint32_t>(index))->getName(7), 8) : 0;
    case 10:
     if (std::isfinite(option) && option >= 8000 && option <= 384000) {
      instance.sampleRate = option;
      if (instance.active) instance.configure();
     }
     return 0;
    case 11:
     if (value > 0 && value <= 1048576) {
      instance.blockSize = static_cast<int>(value);
      if (instance.active) instance.configure();
     }
     return 0;
    case 12:
     instance.active = value != 0;
     if (instance.active) instance.configure(); else processor.releaseResources();
     return 0;
    case 13:
     if (data) *static_cast<vst_rect**>(data) = &instance.rectangle;
     return data ? 1 : 0;
    case 14:
     if (!data) return 0;
     if (!instance.editor) instance.editor.reset(processor.createEditorIfNeeded());
     if (!instance.editor) return 0;
     instance.editor->setBounds(0, 0, 420, 784);
     instance.editor->addToDesktop(0, data);
     instance.editor->setVisible(true);
     return 1;
    case 15: instance.editor.reset(); return 0;
    case 19:
     #if JUCE_LINUX
      for (int i = 0; i < 64 && juce::detail::dispatchNextMessageOnSystemQueue(true); ++i) {}
     #endif
     juce::Timer::callPendingTimersSynchronously();
     return 0;
    case 23:
     if (!data) return 0;
     processor.getStateInformation(instance.chunk);
     *static_cast<void**>(data) = instance.chunk.getData();
     return static_cast<intptr_t>(instance.chunk.getSize());
    case 24:
     if (!data || value <= 0 || value > 16 * 1024 * 1024) return 0;
     { const juce::ScopedValueSetter<bool> guard(receivingHostValue, true);
       processor.setStateInformation(data, static_cast<int>(value)); }
     return 1;
    case 26: return instance.parameter(static_cast<uint32_t>(index)) ? 1 : 0;
    case 29:
     return value >= 0 && value < processor.getNumPrograms()
          ? text(data, processor.getProgramName(static_cast<int>(value)), 24) : 0;
    case 33: case 34: {
     if (!data || index < 0 || index >= (opcode == 33 ? 4 : 2)) return 0;
     struct Pin { char label[64]; int32_t flags, arrangement; char shortLabel[8], reserved[48]; };
     auto& pin = *static_cast<Pin*>(data); std::memset(&pin, 0, sizeof(pin));
     text(pin.label, opcode == 34 ? "Output " + juce::String(index + 1)
                : (index < 2 ? "Vocal " : "Music ") + juce::String(index % 2 + 1), 64);
     pin.flags = 1 | (index % 2 == 0 ? 2 : 0); pin.arrangement = 1;
     return 1;
    }
    case 35: return 1; // Effect category.
    case 44: {
     const auto& parameters = processor.getParameters();
     for (int i = 0; i < parameters.size(); ++i)
      if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameters[i]))
       if (p->paramID == "bypass") set(e, static_cast<uint32_t>(i), value ? 1.0f : 0.0f);
     return 1;
    }
    case 45: return text(data, "Vocal Balance", 32);
    case 47: return text(data, "Independent Audio", 64);
    case 48: return text(data, "Vocal Balance", 64);
    case 49: return legacyVersion;
    case 51:
     if (!data) return 0;
     return std::strcmp(static_cast<const char*>(data), "bypass") == 0
         || std::strcmp(static_cast<const char*>(data), "plugAsChannelInsert") == 0
         || std::strcmp(static_cast<const char*>(data), "plugAsSend") == 0 ? 1 : 0;
    case 52: return 0; // No tail.
    case 58: return 2400;
    case 71: case 72: return 1;
    case 77: return value == 0 ? 1 : 0; // Float processing only.
    default: return 0;
   }
  } catch (...) { return 0; }
 }
 void audioProcessorParameterChanged(juce::AudioProcessor*, int index, float value) override {
  if (!receivingHostValue && host) host(&effect, VST_HOST_OPCODE_00, index, 0, nullptr, value);
 }
 void audioProcessorChanged(juce::AudioProcessor*, const ChangeDetails&) override {}
 void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor*, int index) override {
  if (!receivingHostValue && host) host(&effect, VST_HOST_OPCODE_2B, index, 0, nullptr, 0);
 }
 void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor*, int index) override {
  if (!receivingHostValue && host) host(&effect, VST_HOST_OPCODE_2C, index, 0, nullptr, 0);
 }
};
}
extern "C" VB_EXPORT vst_effect* VST_FUNCTION_INTERFACE VSTPluginMain(vst_host_callback host) {
 if (!host) return nullptr;
 try { return &(new Vst2CompatibilityInstance(host))->effect; } catch (...) { return nullptr; }
}
#if JUCE_WINDOWS
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
 if (reason == DLL_PROCESS_ATTACH) juce::Process::setCurrentModuleInstanceHandle(instance);
 return TRUE;
}
#endif

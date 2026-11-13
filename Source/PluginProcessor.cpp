#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VocalBalanceAudioProcessor::makeParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto number = [&](const char* id, const char* name, float low, float high, float initial, const char* unit) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>{low, high, .1f}, initial,
            juce::AudioParameterFloatAttributes{}.withLabel(unit)));
    };
    number("target", "Target", -42, 0, -21, "dBFS");
    number("lower", "Range Minimum", -12, 11, -6, "dB");
    number("upper", "Range Maximum", -11, 12, 6, "dB");
    number("vocal", "Vocal Sensitivity", -12, 12, 0, "dB");
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"attack", 1}, "Attack", juce::StringArray{"Fast", "Slow"}, 1));
    number("output", "Output", -12, 12, 0, "dB");
    number("music", "Music Sensitivity", -12, 12, 0, "dB");
    number("idle", "Idle", -12, 12, 0, "dB");
    number("rider", "Rider", -12, 12, 0, "dB");
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Automation", juce::StringArray{"Off", "Write", "Read"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false));
    return layout;
}
VocalBalanceAudioProcessor::VocalBalanceAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Vocal", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        .withInput("Music", juce::AudioChannelSet::stereo(), false)),
      state(*this, &history, "VocalBalanceState2", makeParameters()) {
    constexpr const char* ids[] = {"target", "lower", "upper", "vocal", "attack", "output", "music", "idle", "rider", "mode", "bypass"};
    for (size_t i = 0; i < parameters.size(); ++i) parameters[i] = state.getRawParameterValue(ids[i]);
}
VocalBalanceAudioProcessor::~VocalBalanceAudioProcessor() { stopTimer(); }
void VocalBalanceAudioProcessor::startAutomationUpdates() {
    if (juce::MessageManager::getInstanceWithoutCreating()) startTimerHz(30);
}
void VocalBalanceAudioProcessor::prepareToPlay(double rate, int) {
    rider.prepare(rate); startAutomationUpdates();
}
bool VocalBalanceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto main = layout.getMainInputChannelSet();
    if (main != layout.getMainOutputChannelSet() || (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())) return false;
    if (layout.inputBuses.size() > 1) {
        const auto music = layout.inputBuses[1];
        if (!music.isDisabled() && music != juce::AudioChannelSet::mono() && music != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}
void VocalBalanceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    vocalbalance::Controls controls;
    controls.target = parameters[0]->load(); controls.lower = parameters[1]->load(); controls.upper = parameters[2]->load();
    controls.vocal = parameters[3]->load(); controls.fast = parameters[4]->load() < .5f;
    controls.output = parameters[5]->load(); controls.music = parameters[6]->load(); controls.idle = parameters[7]->load();
    controls.rider = parameters[8]->load(); controls.manual = manualRider.load() || parameters[9]->load() > 1.5f;
    controls.bypass = parameters[10]->load() > .5f;
    auto vocal = getBusBuffer(buffer, true, 0);
    auto music = getBusBuffer(buffer, true, 1);
    rider.process(vocal.getArrayOfWritePointers(), vocal.getNumChannels(), vocal.getNumSamples(), controls,
                  music.getArrayOfReadPointers(), music.getNumChannels());
    inputLevel.store(rider.inputDb()); gainDb.store(rider.gainDb()); outputLevel.store(rider.outputDb());
    vocalActive.store(rider.isActive()); musicActive.store(rider.hasMusic());
}
void VocalBalanceAudioProcessor::timerCallback() {
    if (parameters[9]->load() > .5f && parameters[9]->load() < 1.5f && !manualRider.load()) {
        auto* parameter = state.getParameter("rider");
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(gainDb.load()));
        parameter->endChangeGesture();
    }
}
const juce::String VocalBalanceAudioProcessor::getProgramName(int index) {
    return index == 1 ? "Gentle" : index == 2 ? "Firm" : "Default";
}
void VocalBalanceAudioProcessor::setCurrentProgram(int index) {
    const int selected = juce::jlimit(0, 2, index); preset.store(selected);
    const float values[3][8] = {{-21,-6,6,0,1,0,0,0}, {-21,-3,3,0,1,0,0,0}, {-21,-9,9,3,0,0,0,0}};
    constexpr const char* ids[] = {"target","lower","upper","vocal","attack","output","music","idle"};
    for (size_t i = 0; i < 8; ++i) {
        auto* parameter = state.getParameter(ids[i]); parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(values[selected][i])); parameter->endChangeGesture();
    }
}
void VocalBalanceAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    auto tree = state.copyState(); tree.setProperty("schema", 2, nullptr); tree.setProperty("preset", preset.load(), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary(*xml, destination);
}
void VocalBalanceAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName("VocalBalanceState2")) {
        auto tree = juce::ValueTree::fromXml(*xml);
        preset.store(juce::jlimit(0, 2, static_cast<int>(tree.getProperty("preset", 0)))); state.replaceState(tree);
    }
}
juce::AudioProcessorEditor* VocalBalanceAudioProcessor::createEditor() { startAutomationUpdates(); return new VocalBalanceAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocalBalanceAudioProcessor; }

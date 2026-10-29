#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
constexpr float pi = 3.14159265358979323846f;
float midiToHz(float midi) { return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f); }
}

BerniggTuneAudioProcessor::BerniggTuneAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "PARAMETERS", {
        std::make_unique<juce::AudioParameterFloat>("retune", "Retune Speed", 0.0f, 400.0f, 18.0f),
        std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 100.0f, 100.0f),
        std::make_unique<juce::AudioParameterFloat>("output", "Output", -18.0f, 12.0f, 0.0f),
        std::make_unique<juce::AudioParameterChoice>("key", "Key", juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0),
        std::make_unique<juce::AudioParameterChoice>("scale", "Scale", juce::StringArray{"Chromatic","Major","Minor"}, 2),
        std::make_unique<juce::AudioParameterBool>("hard", "Hard Tune", true)
      }) {}

void BerniggTuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    const auto size = static_cast<size_t>(std::ceil(sr * 0.12)) + static_cast<size_t>(samplesPerBlock) + 8;
    for (auto& d : delayBuffers) d.assign(size, 0.0f);
    writePos = 0; phase = 0.0f; smoothedRatio = 1.0f; detectedMidi = -1.0f;
    lastTargetMidi = -1.0f;
}
void BerniggTuneAudioProcessor::releaseResources() {}
bool BerniggTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

float BerniggTuneAudioProcessor::detectMidi(const juce::AudioBuffer<float>& b) const {
    const int n = b.getNumSamples();
    if (n < 512 || sr < 8000.0) return -1.0f;
    const int maxLag = juce::jmin(n / 2, static_cast<int>(sr / 70.0));
    const int minLag = juce::jmax(8, static_cast<int>(sr / 1000.0));
    if (maxLag <= minLag) return -1.0f;
    // Autocorrelation over the central section; mono sum works for typical vocal input.
    const int start = n / 8;
    const int length = juce::jmin(n - start, 1024);
    float best = 0.0f; int bestLag = 0;
    for (int lag = minLag; lag <= maxLag; ++lag) {
        float xy = 0.0f, xx = 0.0f, yy = 0.0f;
        for (int i = 0; i < length - lag; i += 2) {
            float x = 0.0f, y = 0.0f;
            for (int ch = 0; ch < b.getNumChannels(); ++ch) {
                x += b.getSample(ch, start + i);
                y += b.getSample(ch, start + i + lag);
            }
            x /= static_cast<float>(b.getNumChannels()); y /= static_cast<float>(b.getNumChannels());
            xy += x * y; xx += x * x; yy += y * y;
        }
        const float corr = xy / std::sqrt(xx * yy + 1.0e-12f);
        if (corr > best) { best = corr; bestLag = lag; }
    }
    if (bestLag == 0 || best < 0.55f) return -1.0f;
    const float hz = static_cast<float>(sr) / static_cast<float>(bestLag);
    return 69.0f + 12.0f * std::log2(hz / 440.0f);
}

float BerniggTuneAudioProcessor::quantizeMidi(float midi, int key, int scale) const {
    static constexpr int major[] = {0,2,4,5,7,9,11};
    static constexpr int minor[] = {0,2,3,5,7,8,10};
    if (scale == 0) return std::round(midi);
    const int base = static_cast<int>(std::floor(midi));
    float best = midi; float dist = 100.0f;
    for (int note = base - 2; note <= base + 2; ++note) {
        int pc = ((note - key) % 12 + 12) % 12; bool allowed = false;
        for (int j = 0; j < 7; ++j) allowed |= (scale == 1 ? major[j] : minor[j]) == pc;
        if (allowed && std::abs(midi - note) < dist) { dist = std::abs(midi - note); best = static_cast<float>(note); }
    }
    return best;
}

void BerniggTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels(), n = buffer.getNumSamples();
    if (channels == 0 || n == 0 || delayBuffers[0].empty()) return;
    const float retuneMs = params.getRawParameterValue("retune")->load();
    const float mix = params.getRawParameterValue("mix")->load() * 0.01f;
    const float gain = juce::Decibels::decibelsToGain(params.getRawParameterValue("output")->load());
    const int key = static_cast<int>(params.getRawParameterValue("key")->load());
    const int scale = static_cast<int>(params.getRawParameterValue("scale")->load());
    const bool hard = params.getRawParameterValue("hard")->load() > 0.5f;
    const float rawMidi = detectMidi(buffer);
    if (rawMidi >= 0.0f) {
        detectedMidi = rawMidi;
        const float target = quantizeMidi(rawMidi, key, scale);
        const float strength = hard ? 1.0f : 0.72f;
        const float desired = std::pow(2.0f, ((target - rawMidi) * strength) / 12.0f);
        const float time = hard ? 0.002f : juce::jmax(0.002f, retuneMs * 0.001f);
        const float smoothing = 1.0f - std::exp(-static_cast<float>(n) / static_cast<float>(sr * time));
        smoothedRatio += (desired - smoothedRatio) * smoothing;
        lastTargetMidi = target;
    }
    // Two-head modulated delay-line pitch shifter. This is a lightweight MVP;
    // it can produce audible artifacts on complex polyphonic material and is designed for monophonic vocals.
    const float ratio = juce::jlimit(0.5f, 2.0f, smoothedRatio);
    const float range = static_cast<float>(sr * 0.035);
    const float minDelay = static_cast<float>(sr * 0.018);
    const float delta = 1.0f - ratio;
    const float phaseRate = std::abs(delta) / juce::jmax(1.0f, range);
    const float direction = delta >= 0.0f ? 1.0f : -1.0f;
    auto readInterp = [](const std::vector<float>& d, float pos) {
        const float len = static_cast<float>(d.size());
        while (pos < 0.0f) pos += len; while (pos >= len) pos -= len;
        const auto i0 = static_cast<size_t>(pos); const auto i1 = (i0 + 1) % d.size();
        const float frac = pos - static_cast<float>(i0);
        return d[i0] + (d[i1] - d[i0]) * frac;
    };
    for (int i = 0; i < n; ++i) {
        const float p = phase;
        const float fade = p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f;
        for (int ch = 0; ch < channels; ++ch) {
            auto& d = delayBuffers[static_cast<size_t>(ch)];
            const float in = buffer.getSample(ch, i);
            d[writePos] = in;
            const float len = static_cast<float>(d.size());
            const float d1 = minDelay + p * range;
            const float d2 = minDelay + (1.0f - p) * range;
            const float y1 = readInterp(d, static_cast<float>(writePos) - d1 + len);
            const float y2 = readInterp(d, static_cast<float>(writePos) - d2 + len);
            const float shifted = y1 * fade + y2 * (1.0f - fade);
            buffer.setSample(ch, i, (in * (1.0f - mix) + shifted * mix) * gain);
        }
        writePos = (writePos + 1) % delayBuffers[0].size();
        phase += direction * phaseRate;
        phase -= std::floor(phase);
    }
}

juce::AudioProcessorEditor* BerniggTuneAudioProcessor::createEditor() { return new BerniggTuneAudioProcessorEditor(*this); }
void BerniggTuneAudioProcessor::getStateInformation(juce::MemoryBlock& dest) { auto state = params.copyState(); std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, dest); }
void BerniggTuneAudioProcessor::setStateInformation(const void* data, int size) { std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size)); if (xml && xml->hasTagName(params.state.getType())) params.replaceState(juce::ValueTree::fromXml(*xml)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BerniggTuneAudioProcessor(); }

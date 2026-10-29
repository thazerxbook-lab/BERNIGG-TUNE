#pragma once
#include <JuceHeader.h>
#include <vector>

class BerniggTuneAudioProcessor : public juce::AudioProcessor {
public:
    BerniggTuneAudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BERNIGG TUNE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.12; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;
    juce::AudioProcessorValueTreeState params;
    float getDetectedMidi() const { return detectedMidi; }
    float getTargetMidi() const { return lastTargetMidi; }
private:
    float detectMidi(const juce::AudioBuffer<float>&) const;
    float quantizeMidi(float midi, int key, int scale) const;
    double sr = 44100.0;
    std::vector<float> delayBuffers[2];
    size_t writePos = 0;
    float phase = 0.0f, smoothedRatio = 1.0f;
    float detectedMidi = -1.0f, lastTargetMidi = -1.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BerniggTuneAudioProcessor)
};

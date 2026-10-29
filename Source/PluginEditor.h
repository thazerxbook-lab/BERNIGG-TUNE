#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class BerniggTuneAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BerniggTuneAudioProcessorEditor(BerniggTuneAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { repaint(); }
    BerniggTuneAudioProcessor& processor;
    juce::Slider retuneSlider, mixSlider, outputSlider;
    juce::ComboBox keyBox, scaleBox;
    juce::ToggleButton hardButton { "Hard correction" };
    juce::AudioProcessorValueTreeState::SliderAttachment retune, mix, output;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment key, scale;
    juce::AudioProcessorValueTreeState::ButtonAttachment hard;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BerniggTuneAudioProcessorEditor)
};

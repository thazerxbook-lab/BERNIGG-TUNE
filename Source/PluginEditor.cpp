#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
const juce::Colour deepBlue(0xff0649a8), brightBlue(0xff1679ff), neonGreen(0xff9cff35), violet(0xff8e3dff), yellow(0xffffe548), ink(0xff07142a);
void styleKnob(juce::Slider& s) { s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20); s.setColour(juce::Slider::rotarySliderFillColourId, neonGreen); s.setColour(juce::Slider::rotarySliderOutlineColourId, brightBlue); s.setColour(juce::Slider::thumbColourId, yellow); s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white); s.setColour(juce::Slider::textBoxBackgroundColourId, ink); s.setColour(juce::Slider::textBoxOutlineColourId, brightBlue); }
}
BerniggTuneAudioProcessorEditor::BerniggTuneAudioProcessorEditor(BerniggTuneAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      retune(p.params, "retune", retuneSlider), mix(p.params, "mix", mixSlider), output(p.params, "output", outputSlider),
      key(p.params, "key", keyBox), scale(p.params, "scale", scaleBox), hard(p.params, "hard", hardButton) {
    setSize(720, 460);
    styleKnob(retuneSlider); styleKnob(mixSlider); styleKnob(outputSlider);
    retuneSlider.setName("RETUNE SPEED"); mixSlider.setName("MIX"); outputSlider.setName("OUTPUT");
    for (auto* c : {static_cast<juce::Component*>(&retuneSlider), static_cast<juce::Component*>(&mixSlider), static_cast<juce::Component*>(&outputSlider), static_cast<juce::Component*>(&keyBox), static_cast<juce::Component*>(&scaleBox), static_cast<juce::Component*>(&hardButton)}) addAndMakeVisible(c);
    keyBox.setColour(juce::ComboBox::backgroundColourId, ink); keyBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    scaleBox.setColour(juce::ComboBox::backgroundColourId, ink); scaleBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    const juce::StringArray keyNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < keyNames.size(); ++i) keyBox.addItem(keyNames[i], i + 1);
    scaleBox.addItem("Chromatic", 1); scaleBox.addItem("Major", 2); scaleBox.addItem("Minor", 3);
    keyBox.setSelectedId(1, juce::dontSendNotification); scaleBox.setSelectedId(3, juce::dontSendNotification);
    hardButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    retuneSlider.setBounds(45, 230, 150, 150); mixSlider.setBounds(220, 230, 150, 150); outputSlider.setBounds(395, 230, 150, 150);
    keyBox.setBounds(45, 175, 115, 30); scaleBox.setBounds(175, 175, 145, 30); hardButton.setBounds(350, 175, 145, 30);
    startTimerHz(12);
}
void BerniggTuneAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(ink);
    g.setGradientFill(juce::ColourGradient(deepBlue, 0, 0, juce::Colour(0xff102050), getWidth(), getHeight(), false));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(8), 20.0f);
    g.setColour(brightBlue); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8), 20.0f, 3.0f);
    juce::Path beam; beam.startNewSubPath(235, 132); beam.lineTo(475, 132); beam.lineTo(555, 215); beam.lineTo(155, 215); beam.closeSubPath();
    g.setColour(neonGreen.withAlpha(0.18f)); g.fillPath(beam);
    g.setColour(violet); g.fillEllipse(238, 74, 244, 60); g.setColour(juce::Colours::black); g.drawEllipse(238, 74, 244, 60, 4.0f);
    g.setColour(juce::Colour(0xff3b82ff)); g.fillEllipse(300, 30, 120, 66); g.setColour(juce::Colours::black); g.drawEllipse(300, 30, 120, 66, 4.0f);
    g.setColour(neonGreen); g.fillEllipse(342, 37, 36, 48); g.setColour(juce::Colours::black); g.drawEllipse(342, 37, 36, 48, 3.0f);
    g.setColour(juce::Colours::black); g.fillEllipse(351, 54, 5, 10); g.fillEllipse(365, 54, 5, 10);
    g.setColour(yellow); for (int x : {275, 330, 390, 445}) g.fillEllipse(static_cast<float>(x), 98.0f, 19.0f, 9.0f);
    g.setColour(juce::Colours::white); g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold))); g.drawFittedText("BERNIGG TUNE", 24, 18, 260, 40, juce::Justification::left, 1);
    g.setColour(neonGreen); g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold))); g.drawText("ALIEN VOCAL CORRECTION // VST3", 26, 54, 270, 20, juce::Justification::left);
    g.setColour(juce::Colours::white.withAlpha(0.8f)); g.setFont(juce::Font(juce::FontOptions(13.0f)));
    auto noteName = [](float midi) { if (midi < 0) return juce::String("--"); static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; int n = juce::jlimit(0, 127, static_cast<int>(std::round(midi))); return juce::String(names[n % 12]) + juce::String(n / 12 - 1); };
    g.drawText("RETUNE SPEED", 70, 215, 110, 16, juce::Justification::centred);
    g.drawText("MIX", 240, 215, 110, 16, juce::Justification::centred);
    g.drawText("OUTPUT", 415, 215, 110, 16, juce::Justification::centred);
    g.drawText("INPUT NOTE: " + noteName(processor.getDetectedMidi()) + "     TARGET: " + noteName(processor.getTargetMidi()), 26, 410, 460, 24, juce::Justification::left);
    g.setColour(yellow); g.drawText("HARD TUNE", 353, 210, 120, 18, juce::Justification::left);
}
void BerniggTuneAudioProcessorEditor::resized() {}

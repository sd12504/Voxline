#pragma once

#include <JuceHeader.h>

class VoxlineAudioProcessor;

class VoxlineAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::AudioProcessorValueTreeState::Listener,
                                          private juce::AsyncUpdater
{
public:
    explicit VoxlineAudioProcessorEditor(VoxlineAudioProcessor&);
    ~VoxlineAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    void configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void configureButton(juce::ToggleButton& button, const juce::String& text);

    VoxlineAudioProcessor& audioProcessor;
    std::atomic<float> pendingPolishValue { 0.0f };

    juce::Label titleLabel;

    juce::Slider inputGainSlider;
    juce::Slider polishSlider;
    juce::Slider bodySlider;
    juce::Slider claritySlider;
    juce::Slider airSlider;
    juce::Slider smoothSlider;
    juce::Slider compSlider;
    juce::Slider driveSlider;
    juce::Slider outputGainSlider;

    juce::ToggleButton autoGainButton;
    juce::ToggleButton bypassButton;
    juce::ToggleButton listenButton;

    juce::Label inputGainLabel;
    juce::Label polishLabel;
    juce::Label bodyLabel;
    juce::Label clarityLabel;
    juce::Label airLabel;
    juce::Label smoothLabel;
    juce::Label compLabel;
    juce::Label driveLabel;
    juce::Label outputGainLabel;

    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> polishAttachment;
    std::unique_ptr<SliderAttachment> bodyAttachment;
    std::unique_ptr<SliderAttachment> clarityAttachment;
    std::unique_ptr<SliderAttachment> airAttachment;
    std::unique_ptr<SliderAttachment> smoothAttachment;
    std::unique_ptr<SliderAttachment> compAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ButtonAttachment> listenAttachment;
};

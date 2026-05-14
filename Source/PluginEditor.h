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
    void configureHeaderButton(juce::TextButton& button, const juce::String& text);
    void configurePresetButton(juce::TextButton& button, const juce::String& text, bool isActive = false);
    void configureTextLabel(juce::Label& label, const juce::String& text, juce::Justification justification);
    void layoutSliderGroup(juce::Slider& slider,
                           juce::Label& valueLabel,
                           juce::Label& nameLabel,
                           const juce::Rectangle<int>& sliderBounds,
                           const juce::Rectangle<int>& valueBounds,
                           const juce::Rectangle<int>& nameBounds);
    void updateStaticValueLabels();

    VoxlineAudioProcessor& audioProcessor;
    std::atomic<float> pendingPolishValue { 0.0f };

    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::TextButton presetSelectorButton;
    juce::TextButton settingsButton;
    juce::TextButton cleanModeButton;

    juce::Label inputTitleLabel;
    juce::Label toneTitleLabel;
    juce::Label polishTitleLabel;
    juce::Label outputTitleLabel;
    juce::Label peakRmsLabel;
    juce::Label meterNamesLabel;
    juce::Label inputLedDotsLabel;
    juce::Label inputGainValueLabel;
    juce::Label polishValueLabel;
    juce::Label bodyValueLabel;
    juce::Label clarityValueLabel;
    juce::Label airValueLabel;
    juce::Label smoothValueLabel;
    juce::Label compValueLabel;
    juce::Label driveValueLabel;
    juce::Label outputGainValueLabel;

    juce::TextButton cleanPresetButton;
    juce::TextButton warmPresetButton;
    juce::TextButton brightPresetButton;
    juce::TextButton rapPresetButton;
    juce::TextButton abButton;

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

    juce::ProgressBar outputMeter;
    juce::ProgressBar gainReductionMeter;
    double outputMeterValue = 0.82;
    double gainReductionMeterValue = 0.36;

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

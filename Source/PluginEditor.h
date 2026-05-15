#pragma once

#include <JuceHeader.h>
#include "UI/Assets.h"
#include "UI/CustomKnob.h"
#include "UI/HeroKnob.h"
#include "UI/Theme.h"

class VoxlineAudioProcessor;

class VoxlineAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::AudioProcessorValueTreeState::Listener,
                                          private juce::AsyncUpdater,
                                          private juce::KeyListener
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

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    void configureKnob(VoxlineCustomKnob& knob);
    void configureButton(juce::ToggleButton& button, const juce::String& text);
    void configureHeaderButton(juce::TextButton& button, const juce::String& text);
    void configurePresetButton(juce::TextButton& button, const juce::String& text, bool isActive = false);
    void configureTextLabel(juce::Label& label, const juce::String& text, juce::Justification justification);

    void applyTheme(const VoxlineTheme& theme, int index);
    void cycleTheme();

    VoxlineAudioProcessor& audioProcessor;
    std::atomic<float> pendingPolishValue { 0.0f };

    int currentThemeIndex { 0 };

    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::TextButton presetSelectorButton;
    juce::TextButton settingsButton;
    juce::TextButton cleanModeButton;

    juce::ImageComponent logoMark;
    juce::ImageComponent settingsIcon;
    juce::ImageComponent bypassIcon;
    juce::ImageComponent listenIcon;

    juce::Label inputTitleLabel;
    juce::Label toneTitleLabel;
    juce::Label polishTitleLabel;
    juce::Label outputTitleLabel;
    juce::Label peakRmsLabel;
    juce::Label meterNamesLabel;
    juce::Label inputLedDotsLabel;

    juce::TextButton cleanPresetButton;
    juce::TextButton warmPresetButton;
    juce::TextButton brightPresetButton;
    juce::TextButton rapPresetButton;
    juce::TextButton abButton;

    VoxlineCustomKnob inputGainSlider { "Input", juce::Colour(0xffb68cf2) };
    VoxlineHeroKnob polishSlider { "Polish", juce::Colour(0xffe48aa2) };
    VoxlineCustomKnob bodySlider { "Body", juce::Colour(0xfff0b37f) };
    VoxlineCustomKnob claritySlider { "Clarity", juce::Colour(0xffee9b95) };
    VoxlineCustomKnob airSlider { "Air", juce::Colour(0xffa285f1) };
    VoxlineCustomKnob smoothSlider { "Smooth", juce::Colour(0xffdf8b9b) };
    VoxlineCustomKnob compSlider { "Comp", juce::Colour(0xff8e7af0) };
    VoxlineCustomKnob driveSlider { "Drive", juce::Colour(0xffefaa76) };
    VoxlineCustomKnob outputGainSlider { "Out", juce::Colour(0xff9f8ff4) };

    juce::ToggleButton autoGainButton;
    juce::ToggleButton bypassButton;
    juce::ToggleButton listenButton;

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

#pragma once

#include <JuceHeader.h>
#include "UI/Assets.h"
#include "UI/CustomKnob.h"
#include "UI/HeroKnob.h"
#include "UI/Theme.h"
#include "UI/VoxlineMeter.h"

class VoxlineAudioProcessor;
struct ThemeToggleComp;

class VoxlineAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::AudioProcessorValueTreeState::Listener,
                                          private juce::AsyncUpdater,
                                          private juce::KeyListener,
                                          private juce::Button::Listener,
                                          private juce::ComboBox::Listener,
                                          private juce::Timer
{
public:
    explicit VoxlineAudioProcessorEditor(VoxlineAudioProcessor&);
    ~VoxlineAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // A/B compare
    struct ParameterSnapshot
    {
        float inputGain = 0.0f, polish = 0.0f, body = 0.0f, clarity = 0.0f,
              air = 0.0f, smooth = 0.0f, comp = 0.0f, drive = 0.0f, outputGain = 0.0f,
              spaceAmount = 0.0f, spaceType = 0.0f;
        bool autoGain = true, bypass = false, listen = false;
    };

    void cycleTheme();
    int currentThemeIndex { 0 };

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void timerCallback() override;

    void applyPreset(const juce::String& name);
    void captureSnapshot(ParameterSnapshot& snap);
    void applySnapshot(const ParameterSnapshot& snap);
    void toggleAb();

    void configureKnob(VoxlineCustomKnob& knob);
    void configureButton(juce::ToggleButton& button, const juce::String& text);
    void configureHeaderButton(juce::TextButton& button, const juce::String& text);
    void configurePresetButton(juce::TextButton& button, const juce::String& text, bool isActive = false);
    void configureTextLabel(juce::Label& label, const juce::String& text, juce::Justification justification);

    void applyTheme(const VoxlineTheme& theme, int index);
    void loadIconDrawables(bool dark);

    void paintLedDots(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintIcons(juce::Graphics& g);

    VoxlineAudioProcessor& audioProcessor;
    std::atomic<float> pendingPolishValue { 0.0f };

    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::ComboBox presetDropdown;
    std::unique_ptr<ThemeToggleComp> themeToggle;

    juce::Label inputTitleLabel;
    juce::Label toneTitleLabel;
    juce::Label polishTitleLabel;
    juce::Label outputTitleLabel;
    juce::Label peakRmsLabel;
    juce::Label meterNamesLabel;
    juce::Label outputValueLabel;

    juce::ComboBox spaceTypeCombo;
    juce::Label spaceAmountLabel;
    juce::Slider spaceSlider;
    juce::Label footerLabel;

    juce::TextButton abButton;

    VoxlineCustomKnob inputGainSlider { "Input", juce::Colour(0xffb68cf2) };
    VoxlineHeroKnob polishSlider { "", juce::Colour(0xffe48aa2) };
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

    VoxlineLevelMeter outputMeter;
    VoxlineLevelMeter gainReductionMeter;
    // TODO: connect to real processor meter values — DONE via atomic + timer
    float inputLedLevel = 0.0f;

    std::unique_ptr<juce::Drawable> cachedSettingsIcon;
    std::unique_ptr<juce::Drawable> cachedBypassIcon;
    std::unique_ptr<juce::Drawable> cachedListenIcon;

    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> polishAttachment;
    std::unique_ptr<SliderAttachment> bodyAttachment;
    std::unique_ptr<SliderAttachment> clarityAttachment;
    std::unique_ptr<SliderAttachment> airAttachment;
    std::unique_ptr<SliderAttachment> smoothAttachment;
    std::unique_ptr<SliderAttachment> compAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> spaceAttachment;

    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ButtonAttachment> listenAttachment;

    // A/B compare
    ParameterSnapshot snapshotA, snapshotB;
    bool isSlotAActive = true;
    bool applyingSnapshot = false;
};

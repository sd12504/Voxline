#pragma once

#include <JuceHeader.h>
#include "UI/Assets.h"
#include "UI/CustomKnob.h"
#include "UI/HeroKnob.h"
#include "UI/ImageButton.h"
#include "UI/LayoutLoader.h"
#include "UI/Theme.h"
#include "UI/VoxlineMeter.h"

class VoxlineAudioProcessor;

class VoxlineAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::AudioProcessorValueTreeState::Listener,
                                          private juce::AsyncUpdater,
                                          private juce::Button::Listener,
                                          private juce::ComboBox::Listener,
                                          private juce::Slider::Listener,
                                          private juce::Timer
{
public:
    explicit VoxlineAudioProcessorEditor(VoxlineAudioProcessor&);
    ~VoxlineAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // A/B compare
    struct ParameterSnapshot { juce::ValueTree state; };

    void setAdvancedOpen(bool shouldOpen);
    int currentThemeIndex { 1 };
    int selectedEqBand { 0 };  // 0=HPF 1=LOW 2=MUD 3=PRES 4=AIR 5=LPF

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;
    using ComboBoxAttachment = APVTS::ComboBoxAttachment;

    enum class AdvancedSection { eq, comp, deEss, drive, space };

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;

    void applyPreset(const juce::String& name);
    void selectRelativePreset(int delta);
    void saveUserPreset();
    void loadUserPreset();
    void captureSnapshot(ParameterSnapshot& snap);
    void applySnapshot(const ParameterSnapshot& snap);
    void toggleAb();

    void configureKnob(VoxlineCustomKnob& knob);
    void configureButton(juce::ToggleButton& button, const juce::String& text);
    void configureHeaderButton(juce::TextButton& button, const juce::String& text);
    void configurePresetButton(juce::TextButton& button, const juce::String& text, bool isActive = false);
    void configureTextLabel(juce::Label& label, const juce::String& text, juce::Justification justification);

    void applyTheme(const VoxlineTheme& theme);
    void loadIconDrawables();
    void syncEQKnobsToSelectedBand();
    void repaintEQCurve();
    void paintNewInterface(juce::Graphics& g);
    void updateAdvancedVisibility();
    void setAdvancedSection(AdvancedSection section);
    juce::Rectangle<float> getEqGraphBounds() const;
    juce::Point<float> getEqNodePosition(int band) const;
    int findEqNode(juce::Point<float> position) const;
    void updateEqNodeFromMouse(juce::Point<float> position);
    void updateSpectrum();

    void paintLedDots(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintIcons(juce::Graphics& g);

    LayoutLoader layout;
    VoxlineAudioProcessor& audioProcessor;
    std::atomic<float> pendingPolishValue { 0.0f };

    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::ComboBox presetDropdown;
    juce::Label inputTitleLabel;
    juce::Label toneTitleLabel;
    juce::Label polishTitleLabel;
    juce::Label outputTitleLabel;
    juce::Label meterNamesLabel;
    juce::Label spaceTitleLabel;

    juce::ComboBox spaceTypeCombo;
    juce::Label spaceAmountLabel;
    VoxlineCustomKnob spaceSlider { "Space", juce::Colour(0xff8D70E8) };
    juce::Label footerLabel;

    // EQ band buttons (VoxlineImageButton, single-theme)
    VoxlineImageButton eqHpfButton { "HPF" };
    VoxlineImageButton eqLowButton { "LOW" };
    VoxlineImageButton eqMudButton { "MUD" };
    VoxlineImageButton eqPresButton { "PRES" };
    VoxlineImageButton eqAirButton { "AIR" };
    VoxlineImageButton eqLpfButton { "LPF" };

    // Placeholder labels for future controls
    juce::Label thresholdLabel, preDelayLabel, spaceHpfLabel, spaceLpfLabel;
    juce::Label monitorLabel;

    juce::TextButton abButton;
    juce::TextButton presetPreviousButton;
    juce::TextButton presetNextButton;
    juce::TextButton favouriteButton;
    juce::TextButton savePresetButton;
    // Monitor buttons
    juce::TextButton monitorAbBtn;
    juce::TextButton monitorListenBtn;
    juce::TextButton monitorBypassBtn;

    VoxlineCustomKnob inputGainSlider { "Input", juce::Colour(0xffb68cf2) };
    VoxlineHeroKnob polishSlider { "", juce::Colour(0xffe48aa2) };
    VoxlineCustomKnob bodySlider { "Body", juce::Colour(0xfff0b37f) };
    VoxlineCustomKnob claritySlider { "Clarity", juce::Colour(0xffee9b95) };
    VoxlineCustomKnob airSlider { "Air", juce::Colour(0xffa285f1) };
    VoxlineCustomKnob smoothSlider { "Smooth", juce::Colour(0xffdf8b9b) };
    VoxlineCustomKnob compSlider { "Comp", juce::Colour(0xff8e7af0) };
    VoxlineCustomKnob driveSlider { "Drive", juce::Colour(0xffefaa76) };
    VoxlineCustomKnob outputGainSlider { "Out", juce::Colour(0xff9f8ff4) };
    // Placeholder knobs (visual only, no DSP yet)
    VoxlineCustomKnob lowCutKnob { "LOW CUT", juce::Colour(0xffD86F96) };
    VoxlineCustomKnob cleanKnob { "CLEAN", juce::Colour(0xffE99A5C) };
    VoxlineCustomKnob deEssKnob { "DE-ESS", juce::Colour(0xffB8A6F3) };
    VoxlineCustomKnob ratioKnob { "Ratio", juce::Colour(0xffD8A548) };
    VoxlineCustomKnob attackKnob { "Attack", juce::Colour(0xff8D70E8) };
    VoxlineCustomKnob releaseKnob { "Release", juce::Colour(0xffB8A6F3) };
    VoxlineCustomKnob thresholdKnob { "Threshold", juce::Colour(0xffE6B45C) };
    // Space placeholder knobs
    VoxlineCustomKnob preDelayKnob { "PreDelay", juce::Colour(0xffD8A548) };
    VoxlineCustomKnob spaceHpfKnob { "HPF", juce::Colour(0xff8D70E8) };
    VoxlineCustomKnob spaceLpfKnob { "LPF", juce::Colour(0xffB8A6F3) };
    // EQ band control knobs
    VoxlineCustomKnob eqFreqKnob { "FREQ", juce::Colour(0xff8D70E8) };
    VoxlineCustomKnob eqGainKnob { "GAIN", juce::Colour(0xffD8A548) };
    VoxlineCustomKnob eqQKnob { "Q", juce::Colour(0xff8D70E8) };

    // Real Advanced controls. The main-row knobs remain amount macros.
    VoxlineCustomKnob compMixKnob { "Mix", juce::Colour(0xff8D70E8) };
    VoxlineCustomKnob deEssFreqKnob { "Frequency", juce::Colour(0xff8D70E8) };
    VoxlineCustomKnob deEssThresholdKnob { "Threshold", juce::Colour(0xffD8A548) };
    VoxlineCustomKnob deEssRangeKnob { "Range", juce::Colour(0xff8D70E8) };
    VoxlineCustomKnob driveToneKnob { "Tone", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob driveMixKnob { "Mix", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob spaceTimeKnob { "Time", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob spacePreDelayKnob { "Pre-delay", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob spaceWidthKnob { "Width", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob spaceToneKnob { "Tone", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob spaceDecayKnob { "Decay", juce::Colour(0xffD86A35) };
    VoxlineCustomKnob spaceDuckingKnob { "Ducking", juce::Colour(0xffD86A35) };
    juce::ComboBox deEssModeCombo;
    juce::ComboBox driveCharacterCombo;

    juce::TextButton advancedButton;
    juce::TextButton advancedEqButton;
    juce::TextButton advancedCompButton;
    juce::TextButton advancedDeEssButton;
    juce::TextButton advancedDriveButton;
    juce::TextButton advancedSpaceButton;
    juce::TextButton eqResetButton;
    juce::TextButton eqRangeButton;

    VoxlineImageButton autoGainButton { "Auto Gain" };
    VoxlineImageButton   bypassButton { "Bypass" };
    juce::ToggleButton cleanModeButton;
    VoxlineImageButton   listenButton { "Listen" };
    VoxlineImageButton   eqOnButton { "EQ On" };  // EQ bypass toggle

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
    std::unique_ptr<SliderAttachment> compThresholdAttachment;
    std::unique_ptr<SliderAttachment> compRatioAttachment;
    std::unique_ptr<SliderAttachment> compAttackAttachment;
    std::unique_ptr<SliderAttachment> compReleaseAttachment;
    std::unique_ptr<SliderAttachment> compMixAttachment;
    std::unique_ptr<SliderAttachment> deEssFreqAttachment;
    std::unique_ptr<SliderAttachment> deEssThresholdAttachment;
    std::unique_ptr<SliderAttachment> deEssRangeAttachment;
    std::unique_ptr<SliderAttachment> driveToneAttachment;
    std::unique_ptr<SliderAttachment> driveMixAttachment;
    std::unique_ptr<SliderAttachment> spaceTimeAttachment;
    std::unique_ptr<SliderAttachment> spacePreDelayAttachment;
    std::unique_ptr<SliderAttachment> spaceWidthAttachment;
    std::unique_ptr<SliderAttachment> spaceToneAttachment;
    std::unique_ptr<SliderAttachment> spaceDecayAttachment;
    std::unique_ptr<SliderAttachment> spaceDuckingAttachment;
    std::unique_ptr<ComboBoxAttachment> spaceTypeAttachment;
    std::unique_ptr<ComboBoxAttachment> deEssModeAttachment;
    std::unique_ptr<ComboBoxAttachment> driveCharacterAttachment;

    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ButtonAttachment> cleanModeAttachment;
    std::unique_ptr<ButtonAttachment> listenAttachment;
    std::unique_ptr<ButtonAttachment> eqEnabledAttachment;

    // A/B compare
    ParameterSnapshot snapshotA, snapshotB;
    bool isSlotAActive = true;
    bool applyingSnapshot = false;
    bool presetIsFavourite = false;
    bool eqShows24dB = true;
    bool advancedOpen = true;
    AdvancedSection advancedSection = AdvancedSection::eq;
    int draggingEqBand = -1;
    int hoveredEqBand = -1;
    std::unique_ptr<juce::FileChooser> presetFileChooser;
    juce::dsp::FFT spectrumFft;
    juce::dsp::WindowingFunction<float> spectrumWindow;
    std::array<float, 2048> spectrumInput {};
    std::array<float, 4096> spectrumFftData {};
    std::array<float, 256> spectrumDisplay {};
};

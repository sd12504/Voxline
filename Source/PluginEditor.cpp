#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/Layout.h"

VoxlineAudioProcessorEditor::VoxlineAudioProcessorEditor(VoxlineAudioProcessor& audioProcessorToEdit)
    : AudioProcessorEditor(&audioProcessorToEdit),
      audioProcessor(audioProcessorToEdit),
      outputMeter(outputMeterValue),
      gainReductionMeter(gainReductionMeterValue)
{
    auto& apvts = audioProcessor.getAPVTS();

    configureTextLabel(logoLabel, "VOXLINE", juce::Justification::centredLeft);
    configureTextLabel(subtitleLabel, "Fast Vocal Channel", juce::Justification::centredLeft);
    configureHeaderButton(presetSelectorButton, "Preset: Warm");
    configureButton(bypassButton, "BYPASS");
    configureHeaderButton(settingsButton, "...");
    configureHeaderButton(cleanModeButton, "Clean");

    configureTextLabel(inputTitleLabel, "INPUT", juce::Justification::centred);
    configureTextLabel(toneTitleLabel, "TONE CONTROLS", juce::Justification::centred);
    configureTextLabel(polishTitleLabel, "POLISH", juce::Justification::centred);
    configureTextLabel(outputTitleLabel, "OUTPUT", juce::Justification::centred);
    configureTextLabel(peakRmsLabel, "PEAK -3.4\nRMS -14.8", juce::Justification::centred);
    configureTextLabel(meterNamesLabel, "OUT   GR", juce::Justification::centred);
    configureTextLabel(inputLedDotsLabel, "\xe2\x97\x8f \xe2\x97\x8f \xe2\x97\x8f \xe2\x97\x8b \xe2\x97\x8b \xe2\x97\x8b", juce::Justification::centred);

    configurePresetButton(cleanPresetButton, "Clean");
    configurePresetButton(warmPresetButton, "Warm", true);
    configurePresetButton(brightPresetButton, "Bright");
    configurePresetButton(rapPresetButton, "Rap");
    configurePresetButton(abButton, "A/B");

    configureKnob(inputGainSlider);
    configureKnob(polishSlider);
    configureKnob(bodySlider);
    configureKnob(claritySlider);
    configureKnob(airSlider);
    configureKnob(smoothSlider);
    configureKnob(compSlider);
    configureKnob(driveSlider);
    configureKnob(outputGainSlider);

    configureButton(autoGainButton, "Auto Gain");
    configureButton(listenButton, "Listen");

    addAndMakeVisible(logoMark);
    addAndMakeVisible(settingsIcon);
    addAndMakeVisible(bypassIcon);
    addAndMakeVisible(listenIcon);

    outputMeter.setPercentageDisplay(false);
    gainReductionMeter.setPercentageDisplay(false);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(gainReductionMeter);

    inputGainAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::inputGain, inputGainSlider);
    polishAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::polish, polishSlider);
    bodyAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::body, bodySlider);
    clarityAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::clarity, claritySlider);
    airAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::air, airSlider);
    smoothAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::smooth, smoothSlider);
    compAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::comp, compSlider);
    driveAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::drive, driveSlider);
    outputGainAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::outputGain, outputGainSlider);

    autoGainAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::autoGain, autoGainButton);
    bypassAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::bypass, bypassButton);
    listenAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::listen, listenButton);

    apvts.addParameterListener(VoxlineParameterIDs::polish, this);

    setSize(VoxlineLayout::editorWidth, VoxlineLayout::editorHeight);

    // Key listener for theme switching
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Apply default theme (Light)
    applyTheme(VoxlineTheme::light, 0);
}

VoxlineAudioProcessorEditor::~VoxlineAudioProcessorEditor()
{
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::polish, this);
    removeKeyListener(this);
}

void VoxlineAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto& t = VoxlineTheme::get(currentThemeIndex);

    // Editor background
    g.fillAll(t.editorBg);

    // Main card
    g.setColour(t.mainCardBg);
    g.fillRoundedRectangle(VoxlineLayout::mainCard.toFloat(), VoxlineLayout::mainCornerSize);

    // Panel borders (subtle)
    g.setColour(t.shadowLight);
    for (const auto panel : { VoxlineLayout::inputPanel, VoxlineLayout::tonePanel, VoxlineLayout::polishPanel, VoxlineLayout::outputPanel, VoxlineLayout::presetBar })
        g.drawRoundedRectangle(panel.toFloat(), panel == VoxlineLayout::presetBar ? VoxlineLayout::presetBarCornerSize : VoxlineLayout::panelCornerSize, 1.0f);

    // Panel fills
    g.setColour(t.panelBg);
    g.fillRoundedRectangle(VoxlineLayout::inputPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::tonePanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::polishPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::outputPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::presetBar.toFloat(), VoxlineLayout::presetBarCornerSize);
}

void VoxlineAudioProcessorEditor::resized()
{
    logoLabel.setBounds(VoxlineLayout::logoBounds);
    subtitleLabel.setBounds(VoxlineLayout::subtitleBounds);
    presetSelectorButton.setBounds(VoxlineLayout::presetSelectorBounds);
    bypassButton.setBounds(VoxlineLayout::bypassButtonBounds);
    settingsButton.setBounds(VoxlineLayout::settingsButtonBounds);

    inputTitleLabel.setBounds(VoxlineLayout::inputTitleBounds);
    inputGainSlider.setBounds(VoxlineLayout::inputGainSliderBounds);
    autoGainButton.setBounds(VoxlineLayout::autoGainToggleBounds);
    cleanModeButton.setBounds(VoxlineLayout::cleanModeBounds);
    inputLedDotsLabel.setBounds(VoxlineLayout::inputLedDotsBounds);

    toneTitleLabel.setBounds(VoxlineLayout::toneTitleBounds);
    bodySlider.setBounds(VoxlineLayout::bodySliderBounds);
    claritySlider.setBounds(VoxlineLayout::claritySliderBounds);
    airSlider.setBounds(VoxlineLayout::airSliderBounds);
    smoothSlider.setBounds(VoxlineLayout::smoothSliderBounds);
    compSlider.setBounds(VoxlineLayout::compSliderBounds);
    driveSlider.setBounds(VoxlineLayout::driveSliderBounds);

    polishTitleLabel.setBounds(VoxlineLayout::polishTitleBounds);
    polishSlider.setBounds(VoxlineLayout::polishSliderBounds);

    outputTitleLabel.setBounds(VoxlineLayout::outputTitleBounds);
    peakRmsLabel.setBounds(VoxlineLayout::peakRmsBounds);
    outputMeter.setBounds(VoxlineLayout::outMeterBounds);
    gainReductionMeter.setBounds(VoxlineLayout::grMeterBounds);
    meterNamesLabel.setBounds(VoxlineLayout::meterLabelsBounds);
    outputGainSlider.setBounds(VoxlineLayout::outputGainSliderBounds);

    cleanPresetButton.setBounds(VoxlineLayout::cleanPresetBounds);
    warmPresetButton.setBounds(VoxlineLayout::warmPresetBounds);
    brightPresetButton.setBounds(VoxlineLayout::brightPresetBounds);
    rapPresetButton.setBounds(VoxlineLayout::rapPresetBounds);
    abButton.setBounds(VoxlineLayout::abButtonBounds);
    listenButton.setBounds(VoxlineLayout::listenUtilityBounds);

    // Asset icons
    logoMark.setBounds(310, 62, 34, 34);
    settingsIcon.setBounds(1013, 71, 24, 24);
    bypassIcon.setBounds(912, 71, 24, 24);
    listenIcon.setBounds(964, 622, 24, 24);
}

void VoxlineAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID != VoxlineParameterIDs::polish)
        return;

    pendingPolishValue.store(newValue);
    triggerAsyncUpdate();
}

void VoxlineAudioProcessorEditor::handleAsyncUpdate()
{
    DBG("VOXLINE polish changed: " + juce::String(pendingPolishValue.load(), 2));
    repaint();
}

bool VoxlineAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == 't' || key == 'T')
    {
        cycleTheme();
        return true;
    }
    return false;
}

void VoxlineAudioProcessorEditor::applyTheme(const VoxlineTheme& theme, int index)
{
    currentThemeIndex = index;

    // Push theme to all knobs
    inputGainSlider.setTheme(theme);
    polishSlider.setTheme(theme);
    bodySlider.setTheme(theme);
    claritySlider.setTheme(theme);
    airSlider.setTheme(theme);
    smoothSlider.setTheme(theme);
    compSlider.setTheme(theme);
    driveSlider.setTheme(theme);
    outputGainSlider.setTheme(theme);

    // Update label colours
    const auto setTextColour = [&](juce::Label& label)
    {
        label.setColour(juce::Label::textColourId, theme.textPrimary);
    };

    setTextColour(logoLabel);
    setTextColour(subtitleLabel);
    setTextColour(inputTitleLabel);
    setTextColour(toneTitleLabel);
    setTextColour(polishTitleLabel);
    setTextColour(outputTitleLabel);
    setTextColour(peakRmsLabel);
    setTextColour(meterNamesLabel);
    setTextColour(inputLedDotsLabel);

    // Update button colours
    presetSelectorButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);
    settingsButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
    cleanModeButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);
    cleanPresetButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
    warmPresetButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);
    brightPresetButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
    rapPresetButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
    abButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);

    // Toggle buttons
    autoGainButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);
    bypassButton.setColour(juce::ToggleButton::textColourId, theme.textPrimary);
    listenButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);

    // Meter progress bars
    outputMeter.setColour(juce::ProgressBar::backgroundColourId, theme.panelBorder);
    outputMeter.setColour(juce::ProgressBar::foregroundColourId, theme.meterMid);
    gainReductionMeter.setColour(juce::ProgressBar::backgroundColourId, theme.panelBorder);
    gainReductionMeter.setColour(juce::ProgressBar::foregroundColourId, theme.meterLow);

    // Icons (theme-dependent)
    const auto dark = (index != 0);
    logoMark.setImage(VoxlineAssets::loadLogoMark(dark, 34));
    settingsIcon.setImage(VoxlineAssets::loadSettingsIcon(dark, 24));
    bypassIcon.setImage(VoxlineAssets::loadBypassIcon(dark, 24));
    listenIcon.setImage(VoxlineAssets::loadListenIcon(dark, 24));

    repaint();
}

void VoxlineAudioProcessorEditor::cycleTheme()
{
    const auto nextIndex = (currentThemeIndex + 1) % 2;
    const auto& theme = VoxlineTheme::get(nextIndex);
    applyTheme(theme, nextIndex);

    DBG("VOXLINE theme switched to: " << (nextIndex == 0 ? "Light" : "Dark"));
}

void VoxlineAudioProcessorEditor::configureKnob(VoxlineCustomKnob& knob)
{
    addAndMakeVisible(knob);
}

void VoxlineAudioProcessorEditor::configureButton(juce::ToggleButton& button, const juce::String& text)
{
    button.setButtonText(text);
    addAndMakeVisible(button);
}

void VoxlineAudioProcessorEditor::configureHeaderButton(juce::TextButton& button, const juce::String& text)
{
    button.setButtonText(text);
    addAndMakeVisible(button);
}

void VoxlineAudioProcessorEditor::configurePresetButton(juce::TextButton& button, const juce::String& text, bool isActive)
{
    button.setButtonText(text);
    button.setEnabled(false);
    if (isActive)
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xfff3c4c1));
    addAndMakeVisible(button);
}

void VoxlineAudioProcessorEditor::configureTextLabel(juce::Label& label, const juce::String& text, juce::Justification justification)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(justification);
    addAndMakeVisible(label);
}

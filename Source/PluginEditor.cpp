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
    configureTextLabel(inputLedDotsLabel, "● ● ● ○ ○ ○", juce::Justification::centred);

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
}

VoxlineAudioProcessorEditor::~VoxlineAudioProcessorEditor()
{
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::polish, this);
}

void VoxlineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffd9ddd6));

    g.setColour(juce::Colour(0xfff3f1eb));
    g.fillRoundedRectangle(VoxlineLayout::mainCard.toFloat(), VoxlineLayout::mainCornerSize);

    g.setColour(juce::Colour(0x16000000));
    for (const auto panel : { VoxlineLayout::inputPanel, VoxlineLayout::tonePanel, VoxlineLayout::polishPanel, VoxlineLayout::outputPanel, VoxlineLayout::presetBar })
        g.drawRoundedRectangle(panel.toFloat(), panel == VoxlineLayout::presetBar ? VoxlineLayout::presetBarCornerSize : VoxlineLayout::panelCornerSize, 1.0f);

    g.setColour(juce::Colour(0xffece5de));
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

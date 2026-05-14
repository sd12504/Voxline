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
    configureTextLabel(inputGainValueLabel, {}, juce::Justification::centred);
    configureTextLabel(polishValueLabel, {}, juce::Justification::centred);
    configureTextLabel(bodyValueLabel, {}, juce::Justification::centred);
    configureTextLabel(clarityValueLabel, {}, juce::Justification::centred);
    configureTextLabel(airValueLabel, {}, juce::Justification::centred);
    configureTextLabel(smoothValueLabel, {}, juce::Justification::centred);
    configureTextLabel(compValueLabel, {}, juce::Justification::centred);
    configureTextLabel(driveValueLabel, {}, juce::Justification::centred);
    configureTextLabel(outputGainValueLabel, {}, juce::Justification::centred);

    configurePresetButton(cleanPresetButton, "Clean");
    configurePresetButton(warmPresetButton, "Warm", true);
    configurePresetButton(brightPresetButton, "Bright");
    configurePresetButton(rapPresetButton, "Rap");
    configurePresetButton(abButton, "A/B");

    configureSlider(inputGainSlider, inputGainLabel, "Input");
    configureSlider(polishSlider, polishLabel, "Polish");
    configureSlider(bodySlider, bodyLabel, "Body");
    configureSlider(claritySlider, clarityLabel, "Clarity");
    configureSlider(airSlider, airLabel, "Air");
    configureSlider(smoothSlider, smoothLabel, "Smooth");
    configureSlider(compSlider, compLabel, "Comp");
    configureSlider(driveSlider, driveLabel, "Drive");
    configureSlider(outputGainSlider, outputGainLabel, "Output");

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

    updateStaticValueLabels();
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
    layoutSliderGroup(inputGainSlider, inputGainValueLabel, inputGainLabel, VoxlineLayout::inputGainSliderBounds, VoxlineLayout::inputGainValueBounds, VoxlineLayout::inputGainLabelBounds);
    autoGainButton.setBounds(VoxlineLayout::autoGainToggleBounds);
    cleanModeButton.setBounds(VoxlineLayout::cleanModeBounds);
    inputLedDotsLabel.setBounds(VoxlineLayout::inputLedDotsBounds);

    toneTitleLabel.setBounds(VoxlineLayout::toneTitleBounds);
    layoutSliderGroup(bodySlider, bodyValueLabel, bodyLabel, VoxlineLayout::bodySliderBounds, { 252, 340, 118, 22 }, { 252, 360, 118, 18 });
    layoutSliderGroup(claritySlider, clarityValueLabel, clarityLabel, VoxlineLayout::claritySliderBounds, { 372, 340, 118, 22 }, { 372, 360, 118, 18 });
    layoutSliderGroup(airSlider, airValueLabel, airLabel, VoxlineLayout::airSliderBounds, { 492, 340, 118, 22 }, { 492, 360, 118, 18 });
    layoutSliderGroup(smoothSlider, smoothValueLabel, smoothLabel, VoxlineLayout::smoothSliderBounds, { 252, 480, 118, 22 }, { 252, 500, 118, 18 });
    layoutSliderGroup(compSlider, compValueLabel, compLabel, VoxlineLayout::compSliderBounds, { 372, 480, 118, 22 }, { 372, 500, 118, 18 });
    layoutSliderGroup(driveSlider, driveValueLabel, driveLabel, VoxlineLayout::driveSliderBounds, { 492, 480, 118, 22 }, { 492, 500, 118, 18 });

    polishTitleLabel.setBounds(VoxlineLayout::polishTitleBounds);
    layoutSliderGroup(polishSlider, polishValueLabel, polishLabel, VoxlineLayout::polishSliderBounds, VoxlineLayout::polishValueBounds, VoxlineLayout::polishLabelBounds);

    outputTitleLabel.setBounds(VoxlineLayout::outputTitleBounds);
    peakRmsLabel.setBounds(VoxlineLayout::peakRmsBounds);
    outputMeter.setBounds(VoxlineLayout::outMeterBounds);
    gainReductionMeter.setBounds(VoxlineLayout::grMeterBounds);
    meterNamesLabel.setBounds(VoxlineLayout::meterLabelsBounds);
    layoutSliderGroup(outputGainSlider, outputGainValueLabel, outputGainLabel, VoxlineLayout::outputGainSliderBounds, VoxlineLayout::outputValueBounds, VoxlineLayout::outputLabelBounds);

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
    updateStaticValueLabels();
    repaint();
}

void VoxlineAudioProcessorEditor::configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.onValueChange = [this] { updateStaticValueLabels(); };
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
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

void VoxlineAudioProcessorEditor::layoutSliderGroup(juce::Slider& slider,
                                                    juce::Label& valueLabel,
                                                    juce::Label& nameLabel,
                                                    const juce::Rectangle<int>& sliderBounds,
                                                    const juce::Rectangle<int>& valueBounds,
                                                    const juce::Rectangle<int>& nameBounds)
{
    slider.setBounds(sliderBounds);
    valueLabel.setBounds(valueBounds);
    nameLabel.setBounds(nameBounds);
}

void VoxlineAudioProcessorEditor::updateStaticValueLabels()
{
    auto formatSliderValue = [](juce::Slider& slider)
    {
        return slider.getTextFromValue(slider.getValue());
    };

    inputGainValueLabel.setText(formatSliderValue(inputGainSlider), juce::dontSendNotification);
    bodyValueLabel.setText(formatSliderValue(bodySlider), juce::dontSendNotification);
    clarityValueLabel.setText(formatSliderValue(claritySlider), juce::dontSendNotification);
    airValueLabel.setText(formatSliderValue(airSlider), juce::dontSendNotification);
    smoothValueLabel.setText(formatSliderValue(smoothSlider), juce::dontSendNotification);
    compValueLabel.setText(formatSliderValue(compSlider), juce::dontSendNotification);
    driveValueLabel.setText(formatSliderValue(driveSlider), juce::dontSendNotification);
    polishValueLabel.setText(formatSliderValue(polishSlider), juce::dontSendNotification);
    outputGainValueLabel.setText(formatSliderValue(outputGainSlider), juce::dontSendNotification);
}

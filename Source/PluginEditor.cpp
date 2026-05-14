#include "PluginEditor.h"
#include "PluginProcessor.h"

VoxlineAudioProcessorEditor::VoxlineAudioProcessorEditor(VoxlineAudioProcessor& audioProcessorToEdit)
    : AudioProcessorEditor(&audioProcessorToEdit), audioProcessor(audioProcessorToEdit)
{
    auto& apvts = audioProcessor.getAPVTS();

    titleLabel.setText("VOXLINE Phase 2 Debug UI", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

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
    configureButton(bypassButton, "Bypass");
    configureButton(listenButton, "Listen");

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

    setSize(1100, 760);
}

VoxlineAudioProcessorEditor::~VoxlineAudioProcessorEditor()
{
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::polish, this);
}

void VoxlineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void VoxlineAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    titleLabel.setBounds(bounds.removeFromTop(32));
    bounds.removeFromTop(16);

    auto topRow = bounds.removeFromTop(260);
    auto middleRow = bounds.removeFromTop(260);
    auto bottomRow = bounds.removeFromTop(120);

    auto layoutSlider = [](juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider)
    {
        label.setBounds(area.removeFromTop(24));
        area.removeFromTop(8);
        slider.setBounds(area);
    };

    const auto gap = 16;
    const auto sliderWidth = (topRow.getWidth() - gap * 4) / 5;

    auto takeSliderArea = [sliderWidth](juce::Rectangle<int>& row)
    {
        auto area = row.removeFromLeft(sliderWidth);
        row.removeFromLeft(gap);
        return area;
    };

    layoutSlider(takeSliderArea(topRow), inputGainLabel, inputGainSlider);
    layoutSlider(takeSliderArea(topRow), polishLabel, polishSlider);
    layoutSlider(takeSliderArea(topRow), bodyLabel, bodySlider);
    layoutSlider(takeSliderArea(topRow), clarityLabel, claritySlider);
    layoutSlider(takeSliderArea(topRow), airLabel, airSlider);

    layoutSlider(takeSliderArea(middleRow), smoothLabel, smoothSlider);
    layoutSlider(takeSliderArea(middleRow), compLabel, compSlider);
    layoutSlider(takeSliderArea(middleRow), driveLabel, driveSlider);
    layoutSlider(takeSliderArea(middleRow), outputGainLabel, outputGainSlider);

    auto buttonRow = bottomRow.removeFromTop(30);
    auto buttonWidth = 180;
    autoGainButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(20);
    bypassButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(20);
    listenButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
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
}

void VoxlineAudioProcessorEditor::configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
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

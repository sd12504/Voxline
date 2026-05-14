#include "PluginEditor.h"
#include "PluginProcessor.h"

VoxlineAudioProcessorEditor::VoxlineAudioProcessorEditor(VoxlineAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor)
{
    juce::ignoreUnused(audioProcessor);

    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setText("VOXLINE Phase 0 skeleton", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    setSize(640, 320);
}

void VoxlineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("Minimal JUCE plugin shell only. No UI styling, DSP, or controls yet.",
                     getLocalBounds().reduced(24, 24),
                     juce::Justification::centredTop,
                     2);
}

void VoxlineAudioProcessorEditor::resized()
{
    statusLabel.setBounds(getLocalBounds().reduced(24, 24).removeFromBottom(40));
}

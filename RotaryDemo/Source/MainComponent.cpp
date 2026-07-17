#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize(400, 500);

    // --- rotary slider ------------------------------------------------------
    rotarySlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    rotarySlider.setRange(0.0, 1.0, 0.01);
    rotarySlider.setValue(0.65);
    rotarySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    rotarySlider.setLookAndFeel(&lookAndFeel);
    rotarySlider.onValueChange = [this]()
    {
        const auto val = rotarySlider.getValue();
        valueLabel.setText(juce::String(val, 2), juce::dontSendNotification);
    };
    addAndMakeVisible(rotarySlider);

    // --- value label --------------------------------------------------------
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setFont(juce::Font(18.0f).boldened());
    valueLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(58, 48, 54));
    valueLabel.setText(juce::String(rotarySlider.getValue(), 2), juce::dontSendNotification);
    addAndMakeVisible(valueLabel);
}

//==============================================================================
void MainComponent::resized()
{
    auto area = getLocalBounds();

    // rotary in the upper portion
    auto sliderArea = area.removeFromTop(350);
    rotarySlider.setBounds(sliderArea.reduced(100, 40));

    // label below
    valueLabel.setBounds(area.removeFromTop(50).reduced(140, 10));
}

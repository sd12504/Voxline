#include <JuceHeader.h>

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include "../Source/UI/LayoutLoader.h"

namespace
{
class EditorContractTests final : public juce::UnitTest
{
public:
    EditorContractTests()
        : juce::UnitTest("Editor contract", "VOXLINE") {}

    void runTest() override
    {
        beginTest("editor uses the JSON layout and keeps real controls");
        {
            LayoutLoader layout;
            expect(layout.loadFromMemory(BinaryData::layout_json, BinaryData::layout_jsonSize));
            expectEquals(layout.getEditorWidth(), 1080);
            expectEquals(layout.getEditorHeight(), 720);
            expect(layout.hasKey("polishKnob"));
            expect(layout.hasKey("listenButton"));
            expect(layout.hasKey("outMeter"));

            VoxlineAudioProcessor processor;
            VoxlineAudioProcessorEditor editor(processor);

            expectEquals(editor.getWidth(), layout.getEditorWidth());
            expectEquals(editor.getHeight(), 940);

            int sliderCount = 0;
            int buttonCount = 0;
            int meterCount = 0;
            bool foundPolishSlider = false;
            bool foundListenButton = false;
            bool foundOutputMeter = false;

            for (int i = 0; i < editor.getNumChildComponents(); ++i)
            {
                const auto* child = editor.getChildComponent(i);

                if (const auto* slider = dynamic_cast<const juce::Slider*>(child))
                {
                    ++sliderCount;
                    if (slider->getBounds() == juce::Rectangle<int>{466, 116, 148, 148})
                        foundPolishSlider = true;
                }

                if (const auto* button = dynamic_cast<const juce::Button*>(child))
                {
                    ++buttonCount;
                    if (button->getBounds() == juce::Rectangle<int>{783, 21, 96, 34})
                        foundListenButton = true;
                }

                if (const auto* meter = dynamic_cast<const VoxlineLevelMeter*>(child))
                {
                    ++meterCount;
                    if (meter->getBounds() == layout.getBounds("outMeter"))
                        foundOutputMeter = true;
                }
            }

            expectGreaterOrEqual(sliderCount, 9);
            expectGreaterOrEqual(buttonCount, 11);
            expectEquals(meterCount, 2);
            expect(foundPolishSlider);
            expect(foundListenButton);
            expect(foundOutputMeter);

            editor.setAdvancedOpen(true);
            expectEquals(editor.getHeight(), 940);
            editor.setAdvancedOpen(false);
            expectEquals(editor.getHeight(), 720);
        }
    }
};

EditorContractTests editorContractTests;
}

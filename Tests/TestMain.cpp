#include <JuceHeader.h>

#include "TestSupport.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiScope;

    if (argc >= 2
        && (juce::String(argv[1]) == "--render-editor"
            || juce::String(argv[1]) == "--render-editor-advanced"))
    {
        return VoxlineTest::renderEditorScreenshot(
            argc >= 3 ? juce::String(argv[2]) : juce::String(),
            juce::String(argv[1]) == "--render-editor-advanced");
    }

    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult(i);
            result != nullptr && result->failures > 0)
            return 1;

    return 0;
}

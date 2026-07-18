#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/Layout.h"
#include <cmath>
#include <complex>

// ---------------------------------------------------------------------------
// Shared EQ band parameter mappings (index: 0=HPF 1=LOW 2=MUD 3=PRES 4=AIR 5=LPF)
// ---------------------------------------------------------------------------
namespace
{
    constexpr const char* kEqFreqIDs[6] = {
        VoxlineParameterIDs::hpfFreq, VoxlineParameterIDs::lowFreq,
        VoxlineParameterIDs::mudFreq,  VoxlineParameterIDs::presFreq,
        VoxlineParameterIDs::airFreq,  VoxlineParameterIDs::lpfFreq
    };
    constexpr const char* kEqGainIDs[6] = {
        VoxlineParameterIDs::hpfSlope, VoxlineParameterIDs::body,
        VoxlineParameterIDs::mudGain,  VoxlineParameterIDs::clarity,
        VoxlineParameterIDs::air,  VoxlineParameterIDs::lpfSlope
    };
    constexpr const char* kEqQIDs[6] = {
        nullptr, VoxlineParameterIDs::lowQ, VoxlineParameterIDs::mudQ,
        VoxlineParameterIDs::presQ, VoxlineParameterIDs::airQ, nullptr
    };
    constexpr const char* kEqEnabledIDs[6] = {
        VoxlineParameterIDs::hpfEnabled, VoxlineParameterIDs::lowEnabled,
        VoxlineParameterIDs::mudEnabled, VoxlineParameterIDs::presEnabled,
        VoxlineParameterIDs::airEnabled, VoxlineParameterIDs::lpfEnabled
    };

    juce::File userPresetRoot()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ONETAKE").getChildFile("VOXLINE").getChildFile("Presets");
    }

    const char* spaceModeName(int mode)
    {
        constexpr const char* names[] = { "ROOM", "PLATE", "HALL", "SLAP", "WIDTH" };
        return names[juce::jlimit(0, 4, mode)];
    }
} // anonymous namespace

// ---------------------------------------------------------------------------
// Minimal LookAndFeel — hides ToggleButton tick box frame, text only
// ---------------------------------------------------------------------------
struct VoxlineToggleLookAndFeel final : juce::LookAndFeel_V4
{
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced(2.0f);
        const auto on = button.getToggleState();

        // Pill background
        g.setColour(on ? juce::Colour(0xffD86A35).withAlpha(0.20f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle(b, 7.0f);
        g.setColour(on ? juce::Colour(0xffD86A35) : juce::Colour(0xffaaaaaa).withAlpha(0.4f));
        g.drawRoundedRectangle(b, 7.0f, 1.0f);

        g.setFont(juce::FontOptions(12.0f));
        g.setColour(button.findColour(on ? juce::ToggleButton::tickColourId : juce::ToggleButton::textColourId));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
};

static VoxlineToggleLookAndFeel& getToggleLookAndFeel()
{
    static VoxlineToggleLookAndFeel instance;
    return instance;
}

struct VoxlineButtonLookAndFeel final : juce::LookAndFeel_V4
{
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool highlighted, bool down) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const auto active = button.getToggleState()
                         || button.findColour(juce::TextButton::textColourOffId) == juce::Colour(0xffF06A3D);
        const auto accent = juce::Colour(0xffF06A3D);
        auto fill = active ? juce::Colour(0xff241713) : juce::Colour(0xff171818);
        auto border = active ? accent : juce::Colour(0xff55534F);

        if (highlighted)
        {
            fill = active ? juce::Colour(0xff2C1A14) : juce::Colour(0xff202121);
            border = active ? accent.brighter(0.08f) : juce::Colour(0xff77736D);
        }
        if (down)
            fill = active ? juce::Colour(0xff331B14) : juce::Colour(0xff101111);

        g.setColour(juce::Colours::black.withAlpha(0.38f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), 6.0f);
        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(border.withAlpha(button.isEnabled() ? 0.95f : 0.28f));
        g.drawRoundedRectangle(bounds, 6.0f, active ? 1.15f : 0.85f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool highlighted, bool down) override
    {
        const auto active = button.getToggleState()
                         || button.findColour(juce::TextButton::textColourOffId) == juce::Colour(0xffF06A3D);
        auto colour = active ? juce::Colour(0xffF47A50) : juce::Colour(0xffC6BFB5);
        if (highlighted)
            colour = active ? juce::Colour(0xffFF8A5D) : juce::Colour(0xffEEE8DF);
        if (! button.isEnabled())
            colour = colour.withAlpha(0.32f);

        const auto height = button.getHeight();
        const auto fontSize = height <= 30 ? 10.0f : (height <= 36 ? 11.0f : 11.5f);
        g.setColour(colour);
        g.setFont(juce::Font(juce::FontOptions(fontSize, juce::Font::bold))
                      .withExtraKerningFactor(0.08f));
        g.drawText(button.getButtonText().toUpperCase(),
                   button.getLocalBounds().reduced(6, down ? 1 : 0),
                   juce::Justification::centred, false);
    }
};

static VoxlineButtonLookAndFeel& getButtonLookAndFeel()
{
    static VoxlineButtonLookAndFeel instance;
    return instance;
}

// ---------------------------------------------------------------------------
// Pill-style LookAndFeel for Auto Gain toggle
// ---------------------------------------------------------------------------
struct VoxlineAutoGainLNF final : juce::LookAndFeel_V4
{
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool, bool) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced(2.0f);
        const auto on = button.getToggleState();
        const auto dark = currentAutoGainTheme != 0;

        const auto fillOff = dark ? juce::Colour(0xff171818) : juce::Colour(0xffe8e0d4);
        const auto fillOn  = dark ? juce::Colour(0xff271712) : juce::Colour(0xffF2E8DC);
        const auto borderOff = dark ? juce::Colour(0xff3A3936) : juce::Colour(0xffc8bfb4);
        const auto borderOn = juce::Colour(0xffF06A3D);
        const auto textOn  = dark ? juce::Colour(0xffF47A50) : juce::Colour(0xffB84E22);
        const auto textOff = dark ? juce::Colour(0xffA49D94) : juce::Colour(0xff666666);

        g.setColour(on ? fillOn : fillOff);
        g.fillRoundedRectangle(b, 7.0f);
        g.setColour(on ? borderOn : borderOff);
        g.drawRoundedRectangle(b, 7.0f, 1.0f);

        g.setFont(juce::FontOptions(12.0f));
        g.setColour(on ? textOn : textOff);
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    static int currentAutoGainTheme;
};

int VoxlineAutoGainLNF::currentAutoGainTheme = 0;

// ---------------------------------------------------------------------------
// Compact horizontal slider LookAndFeel for SPACE
// ---------------------------------------------------------------------------
struct VoxlineSpaceSliderLNF final : juce::LookAndFeel_V4
{
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float, float,
                          const juce::Slider::SliderStyle, juce::Slider&) override
    {
        const auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
        const auto trackY = b.getCentreY();
        const auto trackH = 2.5f;
        const auto thumbR = 6.0f;
        const auto dark = (spaceSliderTheme != 0);

        // Track background
        g.setColour(dark ? juce::Colour(0xff2a2635) : juce::Colour(0xffe0d8cc));
        g.fillRoundedRectangle(b.getX(), trackY - trackH * 0.5f, (float)w, trackH, 1.5f);

        // Active track
        const auto activeW = sliderPos - b.getX();
        if (activeW > 2.0f)
        {
            g.setColour(dark ? juce::Colour(0xffD86F96) : juce::Colour(0xffc77daa));
            g.fillRoundedRectangle(b.getX(), trackY - trackH * 0.5f, activeW, trackH, 1.5f);
        }

        // Thumb
        g.setColour(dark ? juce::Colour(0xffE98BAA) : juce::Colour(0xffD86F96));
        g.fillEllipse(sliderPos - thumbR, trackY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
        g.setColour(juce::Colour(0x22000000));
        g.drawEllipse(sliderPos - thumbR + 0.5f, trackY - thumbR + 1.0f, thumbR * 2.0f - 1.0f, thumbR * 2.0f - 1.0f, 0.8f);
    }

    static int spaceSliderTheme;
};

int VoxlineSpaceSliderLNF::spaceSliderTheme = 0;

// ---------------------------------------------------------------------------
// Pill-styled ComboBox LookAndFeel for preset dropdown
// ---------------------------------------------------------------------------
struct VoxlinePresetDropdownLNF final : juce::LookAndFeel_V4
{
    void drawComboBox(juce::Graphics& g, int w, int h, bool isDown, int, int, int, int, juce::ComboBox& box) override
    {
        auto& t = VoxlineTheme::get(box.getProperties().getWithDefault("themeIndex", 0));
        const auto dark = (t.editorBg.getBrightness() < 0.3f);

        g.setColour(dark ? juce::Colour(0xff181919) : juce::Colour(0xffece5de));
        g.fillRoundedRectangle(0, 0, (float)w, (float)h, 8.0f);

        g.setColour(isDown ? t.accentRose : t.panelBorder);
        g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1.0f, (float)h - 1.0f, 8.0f, 1.0f);

        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(box.getText(), 14, 0, w - 34, h, juce::Justification::centredLeft, false);

        // ▼ chevron
        juce::Path chevron;
        chevron.addTriangle((float)(w - 24), (float)(h / 2 - 3), (float)(w - 18), (float)(h / 2 + 3), (float)(w - 12), (float)(h / 2 - 3));
        g.setColour(t.textSecondary);
        g.fillPath(chevron);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSep, bool isActive,
                           bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcut, const juce::Drawable* icon, juce::Colour const* textColour) override
    {
        auto& t = VoxlineTheme::get(currentDropdownTheme);
        const auto dark = (t.editorBg.getBrightness() < 0.3f);

        g.setColour(isHighlighted ? t.accentRose.withAlpha(dark ? 0.18f : 0.15f) : (dark ? juce::Colour(0xff181919) : juce::Colour(0xffF7F0E7)));
        g.fillAll();
        g.setColour(isHighlighted ? t.accentRose : t.textPrimary);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(text, 14, 0, area.getWidth() - 28, area.getHeight(), juce::Justification::centredLeft, false);
        juce::ignoreUnused(isSep, isActive, isTicked, hasSubMenu, shortcut, icon, textColour);
    }

    void getIdealPopupMenuItemSize(const juce::String& text, bool, int, int& width, int& height) override
    {
        width = juce::Font(juce::FontOptions(12.0f)).getStringWidth(text) + 28;
        height = 30;
    }

    static int currentDropdownTheme;
};

int VoxlinePresetDropdownLNF::currentDropdownTheme = 0;

static VoxlinePresetDropdownLNF& getDropdownLookAndFeel()
{
    static VoxlinePresetDropdownLNF instance;
    return instance;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
VoxlineAudioProcessorEditor::VoxlineAudioProcessorEditor(VoxlineAudioProcessor& audioProcessorToEdit)
    : AudioProcessorEditor(&audioProcessorToEdit),
      audioProcessor(audioProcessorToEdit),
      userPresetLibrary(userPresetRoot()),
      presetSession(userPresetLibrary, audioProcessorToEdit.getAPVTS(),
                    audioProcessorToEdit.getMonitorState()),
      spectrumFft(VoxlineAudioProcessor::analyzerFftOrder),
      spectrumWindow(VoxlineAudioProcessor::analyzerFftSize,
                     juce::dsp::WindowingFunction<float>::hann, true)
{
    auto& apvts = audioProcessor.getAPVTS();
    spectrumDisplay.fill(-72.0f);
    layout.loadFromMemory(BinaryData::layout_json, BinaryData::layout_jsonSize);

    // ===== V2 Top Bar =====
    configureTextLabel(logoLabel, "VOXLINE", juce::Justification::centredLeft);
    configureTextLabel(subtitleLabel, "Complete Vocal Channel", juce::Justification::centredLeft);

    userPresetLibrary.initialise();

    // User-preset dropdown
    presetDropdown.setLookAndFeel(&getDropdownLookAndFeel());
    presetDropdown.setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    presetDropdown.getProperties().set("themeIndex", 0);
    presetDropdown.addListener(this);
    addAndMakeVisible(presetDropdown);

    for (auto* button : { &presetPreviousButton, &presetNextButton, &presetManageButton, &savePresetButton })
    {
        addAndMakeVisible(*button);
        button->addListener(this);
        button->setLookAndFeel(&getButtonLookAndFeel());
    }
    presetPreviousButton.setButtonText("<");
    presetNextButton.setButtonText(">");
    presetManageButton.setButtonText("...");
    savePresetButton.setButtonText("SAVE AS");
    refreshPresetMenu();

    // ===== V2 Panel Titles =====

    configureTextLabel(inputTitleLabel, "INPUT / CLEAN", juce::Justification::centredLeft);
    configureTextLabel(toneTitleLabel, "VOCAL EQ", juce::Justification::centredLeft);
    configureTextLabel(polishTitleLabel, "POLISH", juce::Justification::centred);
    configureTextLabel(outputTitleLabel, "OUTPUT", juce::Justification::centred);
    configureTextLabel(meterNamesLabel, "DYNAMICS / COLOR", juce::Justification::centred);
    configureTextLabel(spaceTitleLabel, "SPACE", juce::Justification::centredLeft);

    // EQ band buttons — single-theme PNG, radio behavior (handled in buttonClicked)
    auto setupEqBand = [&](VoxlineImageButton& b,
                            const char* normData, int normSize,
                            const char* actData,  int actSize)
    {
        b.setNormalImage(juce::ImageCache::getFromMemory(normData, normSize));
        b.setActiveImage(juce::ImageCache::getFromMemory(actData,  actSize));
        b.setScaleToFit(true);
        b.setClickingTogglesState(false);
        b.addListener(this);
        addAndMakeVisible(b);
    };
    setupEqBand(eqHpfButton, BinaryData::hpf_normal_png,  BinaryData::hpf_normal_pngSize,
                               BinaryData::hpf_active_png, BinaryData::hpf_active_pngSize);
    setupEqBand(eqLowButton, BinaryData::low_normal_png,  BinaryData::low_normal_pngSize,
                               BinaryData::low_active_png, BinaryData::low_active_pngSize);
    setupEqBand(eqMudButton, BinaryData::mud_normal_png,  BinaryData::mud_normal_pngSize,
                               BinaryData::mud_active_png, BinaryData::mud_active_pngSize);
    setupEqBand(eqPresButton, BinaryData::pres_normal_png, BinaryData::pres_normal_pngSize,
                                BinaryData::pres_active_png, BinaryData::pres_active_pngSize);
    setupEqBand(eqAirButton, BinaryData::air_normal_png,  BinaryData::air_normal_pngSize,
                               BinaryData::air_active_png, BinaryData::air_active_pngSize);
    setupEqBand(eqLpfButton, BinaryData::lpf_normal_png,  BinaryData::lpf_normal_pngSize,
                               BinaryData::lpf_active_png, BinaryData::lpf_active_pngSize);
    selectedEqBand = 1;
    eqLowButton.setToggleState(true, juce::dontSendNotification);  // LOW active by default

    // Placeholder controls (UI only, no DSP yet)
    configureTextLabel(thresholdLabel, "", juce::Justification::centred);  // text now drawn in paint()
    configureTextLabel(preDelayLabel, "PRE-DELAY\n15 ms", juce::Justification::centred);
    configureTextLabel(spaceHpfLabel, "HPF\n200 Hz", juce::Justification::centred);
    configureTextLabel(spaceLpfLabel, "LPF\n8.0 kHz", juce::Justification::centred);
    configureTextLabel(monitorLabel, "MONITOR", juce::Justification::centredLeft);

    configurePresetButton(abButton, "A/B");
    abButton.setLookAndFeel(&getButtonLookAndFeel());
    abButton.setButtonText("A");

    abButton.addListener(this);

    // ===== V2 Input / Clean Panel =====
    configureKnob(inputGainSlider);
    inputGainSlider.setShowInternalLabel(false);
    inputGainSlider.setShowInternalValue(false);
    configureKnob(lowCutKnob);
    lowCutKnob.setShowInternalLabel(false);
    lowCutKnob.setShowInternalValue(false);
    lowCutKnob.setRange(40.0, 200.0, 1.0);
    lowCutKnob.setValue(80.0, juce::dontSendNotification);

    configureKnob(cleanKnob);
    cleanKnob.setShowInternalLabel(false);
    cleanKnob.setShowInternalValue(false);
    cleanKnob.setRange(0.0, 100.0, 1.0);
    cleanKnob.setValue(30.0, juce::dontSendNotification);

    configureKnob(deEssKnob);
    deEssKnob.setVisible(false); // replaced by the real SMOOTH/DE-ESS parameter control

    // Dynamics placeholder knobs (visual only, no DSP)
    ratioKnob.setShowInternalLabel(false);
    ratioKnob.setShowInternalValue(false);
    ratioKnob.setRange(1.0, 10.0, 0.1);
    ratioKnob.setValue(3.0, juce::dontSendNotification);

    attackKnob.setShowInternalLabel(false);
    attackKnob.setShowInternalValue(false);
    attackKnob.setRange(0.5, 100.0, 0.5);
    attackKnob.setValue(15.0, juce::dontSendNotification);

    releaseKnob.setShowInternalLabel(false);
    releaseKnob.setShowInternalValue(false);
    releaseKnob.setRange(10.0, 500.0, 1.0);
    releaseKnob.setValue(80.0, juce::dontSendNotification);

    compSlider.setShowInternalLabel(false);
    compSlider.setShowInternalValue(false);
    driveSlider.setShowInternalLabel(false);
    driveSlider.setShowInternalValue(false);

    thresholdKnob.setShowInternalLabel(false);
    thresholdKnob.setShowInternalValue(false);
    thresholdKnob.setRange(-24.0, 0.0, 0.1);
    thresholdKnob.setValue(-18.0, juce::dontSendNotification);
    // ===== V2 POLISH Hero Panel =====
    configureKnob(polishSlider);
    polishSlider.setShowInternalLabel(false);
    polishSlider.setShowInternalValue(false);

    // ===== V2 Output Panel knobs =====
    // Creator-facing tone and de-ess controls.
    configureKnob(bodySlider);
    configureKnob(claritySlider);
    configureKnob(airSlider);
    configureKnob(smoothSlider);
    for (auto* knob : { static_cast<VoxlineSpriteKnob*>(&bodySlider),
                        static_cast<VoxlineSpriteKnob*>(&claritySlider),
                        static_cast<VoxlineSpriteKnob*>(&airSlider),
                        static_cast<VoxlineSpriteKnob*>(&smoothSlider) })
    {
        knob->setShowInternalLabel(false);
        knob->setShowInternalValue(false);
    }
    bodySlider.setDoubleClickReturnValue(true, 0.0);
    claritySlider.setDoubleClickReturnValue(true, 0.0);
    airSlider.setDoubleClickReturnValue(true, 0.0);
    // ===== V2 Dynamics / Color Panel =====
    configureKnob(compSlider);
    configureKnob(driveSlider);
    configureKnob(thresholdKnob);
    configureKnob(ratioKnob);
    configureKnob(attackKnob);
    configureKnob(releaseKnob);
    // Output knob: no internal text, full 70x70 for circle
    configureKnob(outputGainSlider);
    outputGainSlider.setShowInternalLabel(false);
    outputGainSlider.setShowInternalValue(false);

    // ===== V2 Space / Monitor Panel =====
    // SPACE control
    spaceTypeCombo.addItemList({"Room", "Plate", "Hall", "Slap", "Width"}, 1);
    spaceTypeCombo.setSelectedId(1, juce::dontSendNotification);
    spaceTypeCombo.addListener(this);
    spaceTypeCombo.setLookAndFeel(&getDropdownLookAndFeel());
    spaceTypeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    spaceTypeCombo.getProperties().set("themeIndex", 0);
    addAndMakeVisible(spaceTypeCombo);

    // SPACE is a consistent main-row amount knob in the simplified interface.
    configureKnob(spaceSlider);
    spaceSlider.setShowInternalLabel(false);
    spaceSlider.setShowInternalValue(false);

    configureKnob(preDelayKnob);
    configureKnob(spaceHpfKnob);
    configureKnob(spaceLpfKnob);

    preDelayKnob.setShowInternalLabel(false);
    preDelayKnob.setShowInternalValue(false);
    preDelayKnob.setRange(0.0, 100.0, 1.0);
    preDelayKnob.setValue(15.0, juce::dontSendNotification);

    spaceHpfKnob.setShowInternalLabel(false);
    spaceHpfKnob.setShowInternalValue(false);
    spaceHpfKnob.setRange(20.0, 1000.0, 1.0);
    spaceHpfKnob.setValue(200.0, juce::dontSendNotification);

    spaceLpfKnob.setShowInternalLabel(false);
    spaceLpfKnob.setShowInternalValue(false);
    spaceLpfKnob.setRange(500.0, 20000.0, 100.0);
    spaceLpfKnob.setValue(8000.0, juce::dontSendNotification);

    // Old placeholder labels → empty (now drawn in paint())
    preDelayLabel.setText("", juce::dontSendNotification);
    spaceHpfLabel.setText("", juce::dontSendNotification);
    spaceLpfLabel.setText("", juce::dontSendNotification);

    // Monitor section buttons (Space/Monitor panel, V2 visible)
    configurePresetButton(monitorAbBtn, "A/B");
    configurePresetButton(monitorListenBtn, "Listen");
    configurePresetButton(monitorBypassBtn, "Bypass");

    configureTextLabel(spaceAmountLabel, "24%", juce::Justification::centredRight);

    // Footer — V2 visible
    configureTextLabel(footerLabel, "VOXLINE 2.1  |  SADTONY", juce::Justification::centred);

    // V2 labels — drawn via paint() for now; label components kept for text storage
    logoLabel.setVisible(false);
    subtitleLabel.setVisible(false);
    inputTitleLabel.setVisible(false);
    toneTitleLabel.setVisible(false);
    polishTitleLabel.setVisible(false);
    outputTitleLabel.setVisible(false);
    meterNamesLabel.setVisible(false);
    spaceTitleLabel.setVisible(false);
    monitorLabel.setVisible(false);
    // footerLabel stays visible in V2

    // Bypass — theme-aware image button
    addAndMakeVisible(bypassButton);
    bypassButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::bypass_normal_dark_png,  BinaryData::bypass_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::bypass_normal_light_png, BinaryData::bypass_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::bypass_active_dark_png,  BinaryData::bypass_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::bypass_active_light_png, BinaryData::bypass_active_light_pngSize)
    );
    bypassButton.setThemeIndex(0);
    bypassButton.setHitTestInsets(10, 8, 10, 8);
    bypassButton.addListener(this);

    // The old global Listen control is retired; module-local monitor buttons replace it.
    addChildComponent(listenButton);
    listenButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::listen_normal_dark_png,  BinaryData::listen_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::listen_normal_light_png, BinaryData::listen_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::listen_active_dark_png,  BinaryData::listen_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::listen_active_light_png, BinaryData::listen_active_light_pngSize)
    );
    listenButton.setThemeIndex(0);
    listenButton.setHitTestInsets(10, 8, 10, 8);
    listenButton.addListener(this);

    // EQ On/Off has no dedicated artwork yet; keep the attachment alive but do not show it.
    addChildComponent(eqOnButton);
    eqOnButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::on_normal_dark_png,  BinaryData::on_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_normal_light_png, BinaryData::on_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_active_dark_png,  BinaryData::on_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_active_light_png, BinaryData::on_active_light_pngSize)
    );
    eqOnButton.setThemeIndex(0);
    eqOnButton.setToggleState(true, juce::dontSendNotification);
    eqOnButton.setVisible(false);
    eqOnButton.addListener(this);

    // Retired controls remain instantiated only for old layout data; they are never exposed.
    addChildComponent(autoGainButton);
    autoGainButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::on_normal_dark_png,  BinaryData::on_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_normal_light_png, BinaryData::on_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_active_dark_png,  BinaryData::on_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_active_light_png, BinaryData::on_active_light_pngSize)
    );
    autoGainButton.setThemeIndex(0);
    autoGainButton.setHitTestInsets(10, 8, 10, 8);
    autoGainButton.addListener(this);
    configureButton(cleanModeButton, "Clean");
    cleanModeButton.setLookAndFeel(&getToggleLookAndFeel());
    cleanModeButton.setVisible(false);

    // ===== V2 Vocal EQ band controls =====
    configureKnob(eqFreqKnob);
    configureKnob(eqGainKnob);
    eqFreqKnob.setShowInternalLabel(false);
    eqFreqKnob.setShowInternalValue(false);
    eqGainKnob.setShowInternalLabel(false);
    eqGainKnob.setShowInternalValue(false);
    eqFreqKnob.addListener(this);
    eqGainKnob.addListener(this);
    configureKnob(eqQKnob);
    eqQKnob.setShowInternalLabel(false);
    eqQKnob.setShowInternalValue(false);
    eqQKnob.addListener(this);
    for (auto* knob : { &eqFreqKnob, &eqGainKnob, &eqQKnob })
    {
        knob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 96, 24);
        knob->setColour(juce::Slider::textBoxTextColourId, VoxlineTheme::dark.textPrimary);
        knob->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111212));
        knob->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3A3936));
        knob->setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0xffF06A3D).withAlpha(0.35f));
        knob->setVelocityBasedMode(true);
        knob->setVelocityModeParameters(0.45, 1, 0.06, false);
        knob->setMouseDragSensitivity(320);
    }
    syncEQKnobsToSelectedBand();

    // Advanced drawer navigation.
    for (auto* button : { &advancedButton, &advancedEqButton, &advancedCompButton,
                          &advancedDeEssButton, &advancedDriveButton, &advancedSpaceButton })
    {
        addAndMakeVisible(*button);
        button->addListener(this);
        button->setLookAndFeel(&getButtonLookAndFeel());
    }
    advancedButton.setButtonText("ADVANCED  v");
    advancedEqButton.setButtonText("VOCAL EQ");
    advancedCompButton.setButtonText("COMP");
    advancedDeEssButton.setButtonText("DE-ESS");
    advancedDriveButton.setButtonText("DRIVE");
    advancedSpaceButton.setButtonText("SPACE");

    for (auto* button : { &eqResetButton, &eqRangeButton, &eqBandEnableButton,
                          &eqBandSoloButton, &deEssListenButton, &outputClipClearButton })
    {
        addAndMakeVisible(*button);
        button->addListener(this);
        button->setLookAndFeel(&getButtonLookAndFeel());
    }
    eqResetButton.setButtonText("RESET BAND");
    eqRangeButton.setButtonText("RESET ALL");
    eqBandEnableButton.setButtonText("BAND ON");
    eqBandEnableButton.setClickingTogglesState(true);
    eqBandSoloButton.setButtonText("BAND SOLO");
    eqBandSoloButton.setClickingTogglesState(true);
    deEssListenButton.setButtonText("LISTEN S");
    deEssListenButton.setClickingTogglesState(true);
    outputClipClearButton.setButtonText("CLIP CLEAR");

    for (auto* knob : { &compMixKnob, &compMakeupKnob, &deEssFreqKnob, &deEssThresholdKnob,
                        &deEssRangeKnob, &driveToneKnob, &driveMixKnob,
                        &driveOutputTrimKnob,
                        &spaceTimeKnob, &spacePreDelayKnob, &spaceWidthKnob,
                        &spaceToneKnob, &spaceDecayKnob, &spaceDuckingKnob })
    {
        configureKnob(*knob);
        knob->setShowInternalLabel(false);
        knob->setShowInternalValue(false);
    }

    configureButton(compAutoMakeupButton, "AUTO MAKEUP");
    configureButton(driveLevelMatchButton, "LEVEL MATCH");
    configureButton(spaceMonoSafetyButton, "MONO SAFE");
    for (auto* button : { &compAutoMakeupButton, &driveLevelMatchButton, &spaceMonoSafetyButton })
        button->setLookAndFeel(&getToggleLookAndFeel());

    deEssModeCombo.addItemList({"Split", "Wide"}, 1);
    driveCharacterCombo.addItemList({"Clean", "Warm", "Edge"}, 1);
    for (auto* combo : { &deEssModeCombo, &driveCharacterCombo })
    {
        combo->setLookAndFeel(&getDropdownLookAndFeel());
        combo->getProperties().set("themeIndex", 1);
        // The custom LookAndFeel paints the selected text, so JUCE's internal
        // label must be transparent to avoid drawing the value twice.
        combo->setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(*combo);
    }

    bodySlider.setTooltip("Bipolar low-mid tone control. Centre is 0 dB.");
    claritySlider.setTooltip("Bipolar presence control. Centre is 0 dB.");
    airSlider.setTooltip("Bipolar high-shelf control. Centre is 0 dB.");
    smoothSlider.setTooltip("Overall de-esser amount. Detailed controls are in Advanced.");
    compSlider.setTooltip("Overall compression amount. Threshold, ratio, timing and mix are in Advanced.");
    driveSlider.setTooltip("Overall saturation amount. Tone, mix and character are in Advanced.");

    addChildComponent(outputMeter);   // level tracking only, visual drawn in paint()
    addChildComponent(gainReductionMeter);
    // Placeholder floor so meters show visible fill even during silence
    outputMeter.setMinimumLevel(0.35f);
    gainReductionMeter.setMinimumLevel(0.20f);

    inputGainAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::inputGain, inputGainSlider);
    polishAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::polish, polishSlider);
    bodyAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::body, bodySlider);
    clarityAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::clarity, claritySlider);
    airAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::air, airSlider);
    smoothAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::smooth, smoothSlider);
    compAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::comp, compSlider);
    driveAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::drive, driveSlider);
    outputGainAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::outputGain, outputGainSlider);
    spaceAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceAmount, spaceSlider);
    compThresholdAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::compSensitivity, thresholdKnob);
    compRatioAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::compRatio, ratioKnob);
    compAttackAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::compAttack, attackKnob);
    compReleaseAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::compRelease, releaseKnob);
    compMixAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::compMix, compMixKnob);
    compMakeupAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::compMakeup, compMakeupKnob);
    deEssFreqAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::deEssFreq, deEssFreqKnob);
    deEssThresholdAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::deEssThreshold, deEssThresholdKnob);
    deEssRangeAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::deEssRange, deEssRangeKnob);
    driveToneAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::driveTone, driveToneKnob);
    driveMixAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::driveMix, driveMixKnob);
    driveOutputTrimAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::driveOutputTrim, driveOutputTrimKnob);
    spaceTimeAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceSlapTime, spaceTimeKnob);
    spacePreDelayAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spacePreDelay, spacePreDelayKnob);
    spaceWidthAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceWidth, spaceWidthKnob);
    spaceToneAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceTone, spaceToneKnob);
    spaceDecayAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceDecay, spaceDecayKnob);
    spaceDuckingAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceDucking, spaceDuckingKnob);
    spaceTypeAttachment = std::make_unique<ComboBoxAttachment>(apvts, VoxlineParameterIDs::spaceMode, spaceTypeCombo);
    deEssModeAttachment = std::make_unique<ComboBoxAttachment>(apvts, VoxlineParameterIDs::deEssMode, deEssModeCombo);
    driveCharacterAttachment = std::make_unique<ComboBoxAttachment>(apvts, VoxlineParameterIDs::driveCharacter, driveCharacterCombo);
    syncSpaceModeControls();

    bypassAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::bypass, bypassButton);
    eqEnabledAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::eqEnabled, eqOnButton);
    compAutoMakeupAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::compAutoMakeup, compAutoMakeupButton);
    driveLevelMatchAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::driveLevelMatch, driveLevelMatchButton);
    spaceMonoSafetyAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::spaceMonoSafety, spaceMonoSafetyButton);
    eqBandEnabledAttachment = std::make_unique<ButtonAttachment>(apvts, kEqEnabledIDs[selectedEqBand], eqBandEnableButton);

    apvts.addParameterListener(VoxlineParameterIDs::polish, this);
    apvts.addParameterListener(VoxlineParameterIDs::inputGain, this);
    apvts.addParameterListener(VoxlineParameterIDs::body, this);
    apvts.addParameterListener(VoxlineParameterIDs::clarity, this);
    apvts.addParameterListener(VoxlineParameterIDs::air, this);
    apvts.addParameterListener(VoxlineParameterIDs::smooth, this);
    apvts.addParameterListener(VoxlineParameterIDs::comp, this);
    apvts.addParameterListener(VoxlineParameterIDs::drive, this);
    apvts.addParameterListener(VoxlineParameterIDs::outputGain, this);
    apvts.addParameterListener(VoxlineParameterIDs::bypass, this);
    apvts.addParameterListener(VoxlineParameterIDs::spaceAmount, this);
    apvts.addParameterListener(VoxlineParameterIDs::spaceMode, this);
    for (auto* id : { VoxlineParameterIDs::spaceSlapTime, VoxlineParameterIDs::spacePreDelay,
                      VoxlineParameterIDs::spaceWidth, VoxlineParameterIDs::spaceTone,
                      VoxlineParameterIDs::spaceDecay, VoxlineParameterIDs::spaceDucking })
        apvts.addParameterListener(id, this);
    apvts.addParameterListener(VoxlineParameterIDs::eqEnabled, this);
    apvts.addParameterListener(VoxlineParameterIDs::hpfFreq, this);
    apvts.addParameterListener(VoxlineParameterIDs::hpfSlope, this);
    apvts.addParameterListener(VoxlineParameterIDs::lowFreq, this);
    apvts.addParameterListener(VoxlineParameterIDs::lowQ, this);
    apvts.addParameterListener(VoxlineParameterIDs::mudFreq, this);
    apvts.addParameterListener(VoxlineParameterIDs::mudGain, this);
    apvts.addParameterListener(VoxlineParameterIDs::mudQ, this);
    apvts.addParameterListener(VoxlineParameterIDs::presFreq, this);
    apvts.addParameterListener(VoxlineParameterIDs::presQ, this);
    apvts.addParameterListener(VoxlineParameterIDs::airFreq, this);
    apvts.addParameterListener(VoxlineParameterIDs::airQ, this);
    apvts.addParameterListener(VoxlineParameterIDs::lpfFreq, this);
    apvts.addParameterListener(VoxlineParameterIDs::lpfSlope, this);

    setSize(layout.getEditorWidth(), advancedOpen ? 940 : layout.getEditorHeight());
    setResizable(false, false);

    applyTheme(VoxlineTheme::dark);
    updateAdvancedVisibility();
    updateSpectrum();
    startTimerHz(30); // meter refresh
}

VoxlineAudioProcessorEditor::~VoxlineAudioProcessorEditor()
{
    if (presetNameDialog != nullptr)
        presetNameDialog->exitModalState(0);
    presetNameDialog.reset();
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::polish, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::inputGain, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::body, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::clarity, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::air, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::smooth, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::comp, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::drive, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::outputGain, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::bypass, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::spaceAmount, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::spaceMode, this);
    for (auto* id : { VoxlineParameterIDs::spaceSlapTime, VoxlineParameterIDs::spacePreDelay,
                      VoxlineParameterIDs::spaceWidth, VoxlineParameterIDs::spaceTone,
                      VoxlineParameterIDs::spaceDecay, VoxlineParameterIDs::spaceDucking })
        audioProcessor.getAPVTS().removeParameterListener(id, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::eqEnabled, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::hpfFreq, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::hpfSlope, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::lowFreq, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::lowQ, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::mudFreq, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::mudGain, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::mudQ, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::presFreq, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::presQ, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::airFreq, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::airQ, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::lpfFreq, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::lpfSlope, this);
    presetDropdown.removeListener(this);
    presetDropdown.setLookAndFeel(nullptr);
    spaceTypeCombo.removeListener(this);
    spaceTypeCombo.setLookAndFeel(nullptr);
    deEssModeCombo.setLookAndFeel(nullptr);
    driveCharacterCombo.setLookAndFeel(nullptr);
    cleanModeButton.setLookAndFeel(nullptr);
    presetSession.onClose();
    for (auto* button : { &abButton, &presetPreviousButton, &presetNextButton, &presetManageButton,
                          &savePresetButton, &advancedButton, &advancedEqButton,
                          &advancedCompButton, &advancedDeEssButton, &advancedDriveButton,
                          &advancedSpaceButton, &eqResetButton, &eqRangeButton,
                          &eqBandEnableButton, &eqBandSoloButton, &deEssListenButton,
                          &outputClipClearButton,
                          &monitorAbBtn, &monitorListenBtn, &monitorBypassBtn })
        button->setLookAndFeel(nullptr);
    stopTimer();
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::paint(juce::Graphics& g)
{
    paintNewInterface(g);
    return;

#if 0 // Retired V2 paint path. Kept temporarily for layout-reference archaeology.

    const auto& t = VoxlineTheme::get(currentThemeIndex);
    auto bounds = [this](const juce::String& key, juce::Rectangle<int> fallback) {
        const auto r = layout.getBounds(key);
        return r.isEmpty() ? fallback : r;
    };
    const auto bg = juce::ImageCache::getFromMemory(
        currentThemeIndex == 0 ? BinaryData::background_light_png : BinaryData::background_dark_png,
        currentThemeIndex == 0 ? BinaryData::background_light_pngSize : BinaryData::background_dark_pngSize);
    if (bg.isValid())
        g.drawImage(bg, getLocalBounds().toFloat());
    else
        g.fillAll(t.editorBg);

    // ========================================================================
    // V2 Panel background rendering (subtle, theme-aware)
    // ========================================================================
    {
        const auto dark = (currentThemeIndex != 0);
        const auto panelFill   = dark ? juce::Colour(0xff181622) : juce::Colour(0xffF7F0E7);
        const auto panelBorder = dark ? juce::Colour(0xff2A2635) : juce::Colour(0xffDED3C5);

        auto drawPanel = [&](juce::Rectangle<int> r)
        {
            g.setColour(panelFill);
            g.fillRoundedRectangle(r.toFloat(), VoxlineLayout::panelCornerSize);
            g.setColour(panelBorder);
            g.drawRoundedRectangle(r.toFloat().reduced(0.5f), VoxlineLayout::panelCornerSize, 1.0f);
        };

        // Upper Row panels
        drawPanel(VoxlineLayout::inputPanel);
        drawPanel(VoxlineLayout::polishPanel);
        drawPanel(VoxlineLayout::outputPanel);

        // Lower Row panels
        drawPanel(VoxlineLayout::eqPanel);
        drawPanel(VoxlineLayout::dynamicsPanel);
        drawPanel(VoxlineLayout::spacePanel);
    }

    // LED dots
    paintLedDots(g, bounds("inputLedDots", VoxlineLayout::inputLedDotsBounds));

    // Input panel upper section — split left/right
    g.setColour(t.textSecondary);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("AUTO GAIN", autoGainButton.getBounds().translated(0, -22),
               juce::Justification::centred, false);

    // Gain value
    g.setColour(t.textPrimary);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText(inputGainSlider.getTextFromValue(inputGainSlider.getValue()),
               bounds("inputGainValue", VoxlineLayout::inputGainValueBounds), juce::Justification::centred, false);

    // Input level label + dB value — centered on right half
    g.setColour(t.textSecondary);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    const auto inputLedDotsBounds = bounds("inputLedDots", VoxlineLayout::inputLedDotsBounds);
    g.drawText("INPUT LEVEL", inputLedDotsBounds.expanded(12, 4).translated(0, -22),
               juce::Justification::centred, false);
    g.setColour(t.textMuted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("-18.4 dB", inputLedDotsBounds.expanded(12, 4).translated(0, 14),
               juce::Justification::centred, false);

    // Input panel knob labels (drawn externally because knobs are too small)
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    auto drawKnobLabel = [&](juce::Rectangle<int> r, const juce::String& label, const juce::String& value) {
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(label, r.getX(), r.getY() - 30, r.getWidth(), 16, juce::Justification::centred, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(value, r.getX(), r.getBottom() - 4, r.getWidth(), 18, juce::Justification::centred, false);
    };
    drawKnobLabel(bounds("lowCutKnob", VoxlineLayout::lowCutKnobBounds), "LOW CUT", "80 Hz");
    drawKnobLabel(bounds("cleanKnob", VoxlineLayout::cleanKnobBounds), "CLEAN", "30%");
    drawKnobLabel(bounds("deEssKnob", VoxlineLayout::deEssKnobBounds), "DE-ESS", "25%");

    // POLISH status + description
    {
        const auto val = juce::jlimit(0.0, 100.0, polishSlider.getValue());
        const juce::String status = val <= 33.0 ? "Natural" : (val <= 66.0 ? "Pushed" : "Intense");
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
        g.drawText(polishSlider.getTextFromValue(val), bounds("polishValue", VoxlineLayout::polishValueBounds), juce::Justification::centred, false);
        const auto statusBounds = bounds("polishStatus", VoxlineLayout::polishStatusBounds);
        const auto dark = currentThemeIndex != 0;
        const auto polishAccent = dark ? juce::Colour(0xffFF8A4C) : juce::Colour(0xffD86A35);
        g.setColour(dark ? juce::Colour(0xff241714) : juce::Colour(0xffF7E6D8));
        g.fillRoundedRectangle(statusBounds.toFloat(), 8.0f);
        g.setColour(polishAccent.withAlpha(dark ? 0.75f : 0.65f));
        g.drawRoundedRectangle(statusBounds.toFloat().reduced(0.5f), 8.0f, 1.0f);
        g.setColour(dark ? polishAccent : juce::Colour(0xffB84E22));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(status, statusBounds, juce::Justification::centred, false);
    }

    // Output panel
    {
        const auto& t = VoxlineTheme::get(currentThemeIndex);

        // -- Left: PEAK / RMS readout --
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("PEAK", VoxlineLayout::peakLabelBounds, juce::Justification::centredLeft, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("-60.0 dB", VoxlineLayout::peakValueBounds, juce::Justification::centredLeft, false);

        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("RMS", VoxlineLayout::rmsLabelBounds, juce::Justification::centredLeft, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("-60.0 dB", VoxlineLayout::rmsValueBounds, juce::Justification::centredLeft, false);

        // -- OUT / GR meters (drawn directly, VoxlineLevelMeter is now invisible) --
        {
            const float outLevel = 0.35f, grLevel = 0.20f;
            const float outPeak = 0.42f, grPeak = 0.25f;
            const auto dark = (currentThemeIndex != 0);

            auto drawMeter = [&](juce::Rectangle<int> bounds, float level, float peak, juce::Colour fillColour)
            {
                const auto well = bounds.toFloat().reduced(2.0f);
                // 1. Well background
                g.setColour(dark ? juce::Colour(0xff14121A) : juce::Colour(0xffD5CFC8));
                g.fillRoundedRectangle(well, 5.0f);
                // 2. Fill from bottom
                const auto fillH = well.getHeight() * level;
                if (fillH > 0.5f)
                {
                    const auto fillR = well.withTop(well.getBottom() - fillH);
                    g.setColour(fillColour);
                    g.fillRoundedRectangle(fillR, 4.0f);
                }
                // 3. Peak hold line
                if (peak > 0.005f)
                {
                    const auto peakY = well.getBottom() - well.getHeight() * peak;
                    g.setColour(fillColour.brighter(0.3f));
                    g.drawLine(well.getX() + 2.0f, peakY, well.getRight() - 2.0f, peakY, 1.5f);
                }
                // 4. Border
                g.setColour(t.panelBorder);
                g.drawRoundedRectangle(well.reduced(0.5f), 4.0f, 1.0f);
            };

            drawMeter(bounds("outMeter", VoxlineLayout::outMeterBounds), outLevel, outPeak, juce::Colour(0xffD86A35).withAlpha(0.75f));
            drawMeter(bounds("grMeter", VoxlineLayout::grMeterBounds),  grLevel,  grPeak,  juce::Colour(0xff7A55FF).withAlpha(0.70f));
        }

        // -- Meter labels (below meters) --
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("OUT", bounds("outMeter", VoxlineLayout::outMeterBounds).translated(0, -22), juce::Justification::centred, false);
        g.drawText("GR",  bounds("grMeter", VoxlineLayout::grMeterBounds).translated(0, -22), juce::Justification::centred, false);

        // -- Soft Clip pill button --
        {
            const auto r = bounds("softClip", VoxlineLayout::softClipBounds);
            const auto dark = (currentThemeIndex != 0);
            g.setColour(dark ? juce::Colour(0xff1E1B2A) : juce::Colour(0xffF0EBE4));
            g.fillRoundedRectangle(r.toFloat(), 10.0f);
            g.setColour(t.panelBorder);
            g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 10.0f, 1.0f);
            g.setColour(t.textSecondary);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawFittedText("SOFT CLIP", r, juce::Justification::centred, 1);
        }

        // -- Right: OUTPUT GAIN label + value --
        g.setColour(t.textSecondary);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawFittedText("OUTPUT GAIN", outputGainSlider.getBounds().withHeight(16).translated(0, -22),
                         juce::Justification::centred, 1);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(outputGainSlider.getTextFromValue(outputGainSlider.getValue()),
                   bounds("outputGainValue", VoxlineLayout::outputGainValueBounds), juce::Justification::centred, false);
    }

    // Vocal EQ response display, driven by the six EQ band parameters.
    {
        const auto cb = bounds("eqCurve", VoxlineLayout::eqCurveBounds).toFloat();
        const auto dark = (currentThemeIndex != 0);
        const auto& t = VoxlineTheme::get(currentThemeIndex);
        const auto cbI = cb.toNearestInt();
        auto& apvts = audioProcessor.getAPVTS();

        auto raw = [&](const juce::String& id, float fallback) -> float
        {
            if (auto* v = apvts.getRawParameterValue(id))
                return v->load();
            return fallback;
        };

        const float hpfFreq  = raw(VoxlineParameterIDs::hpfFreq, 80.0f);
        const int   hpfIndex = juce::roundToInt(raw(VoxlineParameterIDs::hpfSlope, 1.0f));
        const float hpfSlope = hpfIndex == 0 ? 12.0f : (hpfIndex == 1 ? 24.0f : 36.0f);
        const float lowFreq  = raw(VoxlineParameterIDs::lowFreq, 160.0f);
        const float lowGain  = raw(VoxlineParameterIDs::lowGain, 1.5f);
        const float lowQ     = raw(VoxlineParameterIDs::lowQ, 0.8f);
        const float mudFreq  = raw(VoxlineParameterIDs::mudFreq, 350.0f);
        const float mudGain  = raw(VoxlineParameterIDs::mudGain, -2.0f);
        const float mudQ     = raw(VoxlineParameterIDs::mudQ, 1.1f);
        const float presFreq = raw(VoxlineParameterIDs::presFreq, 2500.0f);
        const float presGain = raw(VoxlineParameterIDs::presGain, 2.0f);
        const float presQ    = raw(VoxlineParameterIDs::presQ, 1.0f);
        const float airFreq  = raw(VoxlineParameterIDs::airFreq, 10000.0f);
        const float airGain  = raw(VoxlineParameterIDs::airGain, 1.5f);
        const float airQ     = raw(VoxlineParameterIDs::airQ, 0.7f);
        const float lpfFreq  = raw(VoxlineParameterIDs::lpfFreq, 18000.0f);
        const int   lpfIndex = juce::roundToInt(raw(VoxlineParameterIDs::lpfSlope, 0.0f));
        const float lpfSlope = lpfIndex == 0 ? 12.0f : 24.0f;
        const bool  eqOn     = raw(VoxlineParameterIDs::eqEnabled, 1.0f) >= 0.5f;

        const auto curveBg = dark ? juce::Colour(0xff100E18) : juce::Colour(0xff211E29);
        g.setColour(curveBg);
        g.fillRoundedRectangle(cb, 8.0f);

        g.saveState();
        g.reduceClipRegion(cbI);

        const float logMin = std::log10(20.0f), logMax = std::log10(20000.0f);
        const float x0 = cb.getX() + 18.0f, xW = cb.getWidth() - 36.0f;
        const float yTop = cb.getY() + 14.0f, yBot = cb.getBottom() - 14.0f;
        const float yMid = cb.getCentreY();
        const float yScale = (cb.getHeight() - 28.0f) / 24.0f;

        const auto gridMajor = dark ? t.panelBorder : juce::Colour(0xff4A4352);
        const auto gridMinor = dark ? juce::Colour(0xff1E1B2A) : juce::Colour(0xff332E3C);
        const float minorHz[] = { 20,30,40,50,60,80,100,200,300,400,500,600,800,1000,2000,3000,4000,5000,6000,8000,10000,20000 };
        for (auto f : minorHz)
        {
            const float x = x0 + xW * (std::log10(f) - logMin) / (logMax - logMin);
            const bool major = (f == 20 || f == 50 || f == 100 || f == 200 || f == 500 || f == 1000 || f == 2000 || f == 5000 || f == 10000 || f == 20000);
            g.setColour(major ? gridMajor.withAlpha(0.65f) : gridMinor.withAlpha(0.55f));
            g.drawVerticalLine(juce::roundToInt(x), juce::roundToInt(yTop), juce::roundToInt(yBot));
        }
        for (int db = -12; db <= 12; db += 6)
        {
            const float y = yMid - (float)db * yScale;
            g.setColour(db == 0 ? gridMajor.withAlpha(0.95f) : gridMinor.withAlpha(0.65f));
            g.drawHorizontalLine(juce::roundToInt(y), juce::roundToInt(x0 - 6.0f), juce::roundToInt(x0 + xW + 6.0f));
        }

        auto toX = [&](float hz) { return x0 + xW * (std::log10(juce::jlimit(20.0f, 20000.0f, hz)) - logMin) / (logMax - logMin); };
        auto toY = [&](float db) { return juce::jlimit(yTop, yBot, yMid - juce::jlimit(-12.0f, 12.0f, db) * yScale); };
        auto bellDb = [](float hz, float freq, float gain, float q)
        {
            const float w = hz / juce::jmax(1.0f, freq);
            const float shape = (w - 1.0f / w) / juce::jmax(0.2f, q);
            return gain / (1.0f + shape * shape);
        };
        auto shelfDb = [](float hz, float freq, float gain, float q)
        {
            const float w = hz / juce::jmax(1.0f, freq);
            const float p = juce::jlimit(0.35f, 2.5f, q);
            const float shaped = std::pow(w, p * 2.0f);
            return gain * (shaped / (1.0f + shaped));
        };
        auto responseDb = [&](float hz)
        {
            if (! eqOn)
                return 0.0f;

            float resp = 0.0f;
            if (hz < hpfFreq)
                resp -= hpfSlope * std::log2(hpfFreq / juce::jmax(1.0f, hz));
            resp += bellDb(hz, lowFreq, lowGain, lowQ);
            resp += bellDb(hz, mudFreq, mudGain, mudQ);
            resp += bellDb(hz, presFreq, presGain, presQ);
            resp += shelfDb(hz, airFreq, airGain, airQ);
            if (hz > lpfFreq)
                resp -= lpfSlope * std::log2(hz / juce::jmax(1.0f, lpfFreq));
            return resp;
        };

        juce::Path eqPath;
        const int steps = 160;
        for (int i = 0; i <= steps; ++i)
        {
            const float hz = 20.0f * std::pow(1000.0f, (float)i / (float)steps);
            const float px = toX(hz), py = toY(responseDb(hz));
            if (i == 0) eqPath.startNewSubPath(px, py);
            else        eqPath.lineTo(px, py);
        }

        const auto orange = dark ? juce::Colour(0xffFF8A4C) : juce::Colour(0xffD86A35);
        const auto white = dark ? juce::Colour(0xffF5F0EA) : juce::Colour(0xffFFF9F2);
        g.setColour(orange.withAlpha(eqOn ? 0.20f : 0.08f));
        g.strokePath(eqPath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved));
        g.setColour(white.withAlpha(eqOn ? 0.95f : 0.45f));
        g.strokePath(eqPath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
        g.setColour(orange.withAlpha(eqOn ? 0.92f : 0.38f));
        g.strokePath(eqPath, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved));

        struct BandDot { float hz; float db; int idx; juce::Colour col; };
        const BandDot dots[] = {
            { hpfFreq,  responseDb(hpfFreq),  0, orange },
            { lowFreq,  responseDb(lowFreq),  1, dark ? juce::Colour(0xffE8E4F0) : juce::Colour(0xffF5F2FA) },
            { mudFreq,  responseDb(mudFreq),  2, t.accentAmber },
            { presFreq, responseDb(presFreq), 3, t.accentPeach },
            { airFreq,  responseDb(airFreq),  4, t.accentLavender },
            { lpfFreq,  responseDb(lpfFreq),  5, orange.withAlpha(0.9f) },
        };
        for (auto& d : dots)
        {
            const auto cx = toX(d.hz), cy = toY(d.db);
            if (d.idx == selectedEqBand)
            {
                g.setColour(d.col.withAlpha(dark ? 0.35f : 0.28f));
                g.fillEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
                g.setColour(white.withAlpha(0.75f));
                g.drawEllipse(cx - 8.5f, cy - 8.5f, 17.0f, 17.0f, 1.25f);
            }
            g.setColour(d.col.withAlpha(eqOn ? 1.0f : 0.42f));
            g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
        }

        g.setColour(dark ? t.textMuted : juce::Colour(0xffAFA6B6));
        g.setFont(juce::FontOptions(8.0f));
        for (auto f : { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
        {
            const float x = toX((float)f);
            const juce::String label = (f >= 1000) ? juce::String(f / 1000) + "k" : juce::String(f);
            g.drawText(label, juce::roundToInt(x - 15.0f), juce::roundToInt(yBot + 2.0f), 30, 12, juce::Justification::centred, false);
        }

        g.restoreState();

        g.setColour(dark ? t.panelBorder : juce::Colour(0xff3A3645));
        g.drawRoundedRectangle(cb.reduced(0.5f), 8.0f, 1.0f);
    }

    // Selected band control row
    {
        const auto& t = VoxlineTheme::get(currentThemeIndex);
        const auto dark = (currentThemeIndex != 0);
        const char* bandNames[] = { "HPF", "LOW", "MUD", "PRES", "AIR", "LPF" };
        const juce::Colour bandCols[] = {
            dark ? juce::Colour(0xffFF8A4C) : juce::Colour(0xffD86A35),
            dark ? juce::Colour(0xffE8E4F0) : juce::Colour(0xffF5F2FA),
            t.accentAmber,
            t.accentPeach,
            t.accentLavender,
            dark ? juce::Colour(0xffFF8A4C) : juce::Colour(0xffD86A35),
        };
        const int sel = juce::jlimit(0, 5, selectedEqBand);
        const bool isBell = (sel >= 1 && sel <= 4);

        auto& apvts = audioProcessor.getAPVTS();
        auto raw = [&](const juce::String& id, float fallback) -> float
        {
            if (auto* v = apvts.getRawParameterValue(id))
                return v->load();
            return fallback;
        };
        auto freqText = [](float hz)
        {
            return hz >= 1000.0f ? juce::String(hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + " kHz"
                                  : juce::String(juce::roundToInt(hz)) + " Hz";
        };
        auto gainText = [](float db)
        {
            return juce::String(db > 0.0f ? "+" : "") + juce::String(db, 1) + " dB";
        };

        const float freqs[] = {
            raw(VoxlineParameterIDs::hpfFreq, 80.0f),
            raw(VoxlineParameterIDs::lowFreq, 160.0f),
            raw(VoxlineParameterIDs::mudFreq, 350.0f),
            raw(VoxlineParameterIDs::presFreq, 2500.0f),
            raw(VoxlineParameterIDs::airFreq, 10000.0f),
            raw(VoxlineParameterIDs::lpfFreq, 18000.0f),
        };
        const float gains[] = {
            raw(VoxlineParameterIDs::hpfSlope, 1.0f),
            raw(VoxlineParameterIDs::lowGain, 1.5f),
            raw(VoxlineParameterIDs::mudGain, -2.0f),
            raw(VoxlineParameterIDs::presGain, 2.0f),
            raw(VoxlineParameterIDs::airGain, 1.5f),
            raw(VoxlineParameterIDs::lpfSlope, 0.0f),
        };
        const juce::String freqValue = freqText(freqs[sel]);
        const juce::String gainValue = isBell ? gainText(gains[sel])
                                              : juce::String(sel == 0
                                                    ? (juce::roundToInt(gains[sel]) == 0 ? 12 : (juce::roundToInt(gains[sel]) == 1 ? 24 : 36))
                                                    : (juce::roundToInt(gains[sel]) == 0 ? 12 : 24)) + " dB/oct";

        // Band pill
        const auto r1 = VoxlineLayout::eqSelBandBtnBounds.toFloat();
        g.setColour(bandCols[sel].withAlpha(dark ? 0.25f : 0.18f));
        g.fillRoundedRectangle(r1, 8.0f);
        g.setColour(bandCols[sel]);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(bandNames[sel], VoxlineLayout::eqSelBandBtnBounds, juce::Justification::centred, false);

        // FREQ
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("FREQ", VoxlineLayout::eqFreqLabelBounds, juce::Justification::centred, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(freqValue, VoxlineLayout::eqFreqValueBounds, juce::Justification::centred, false);

        // GAIN / SLOPE
        const char* gLabel = isBell ? "GAIN" : "SLOPE";
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(gLabel, VoxlineLayout::eqQLabelBounds, juce::Justification::centred, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(gainValue, VoxlineLayout::eqQValueBounds, juce::Justification::centred, false);

        // RESET
        const auto rr = VoxlineLayout::eqResetBounds.toFloat();
        g.setColour(t.panelBg);
        g.fillRoundedRectangle(rr, 8.0f);
        g.setColour(t.panelBorder);
        g.drawRoundedRectangle(rr.reduced(0.5f), 8.0f, 1.0f);
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("RESET", VoxlineLayout::eqResetBounds, juce::Justification::centred, false);
    }

    // Dynamics / Color panel
    {
        const auto& t = VoxlineTheme::get(currentThemeIndex);
        const auto dark = (currentThemeIndex != 0);

        // -- GR meter (between COMP and THRESHOLD) --
        {
            const auto well = VoxlineLayout::dynamicsGrMeterBounds.toFloat().reduced(2.0f);
            const float grFill = 0.25f, grPeak = 0.32f;

            g.setColour(dark ? juce::Colour(0xff14121A) : juce::Colour(0xffD5CFC8));
            g.fillRoundedRectangle(well, 4.0f);

            const auto fillH = well.getHeight() * grFill;
            if (fillH > 0.5f)
            {
                const auto fillR = well.withTop(well.getBottom() - fillH);
                g.setColour(t.accentPurple);
                g.fillRoundedRectangle(fillR, 3.0f);
            }
            if (grPeak > 0.005f)
            {
                const auto peakY = well.getBottom() - well.getHeight() * grPeak;
                g.setColour(t.accentPurple.brighter(0.3f));
                g.drawLine(well.getX() + 1.0f, peakY, well.getRight() - 1.0f, peakY, 1.5f);
            }
            g.setColour(t.panelBorder);
            g.drawRoundedRectangle(well.reduced(0.5f), 4.0f, 1.0f);
        }

        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("GR", VoxlineLayout::dynamicsGrLabelBounds, juce::Justification::centred, false);

        // -- Row 1 labels & values (11px label, 12px value) --
        auto drawRow1 = [&](juce::Rectangle<int> lr, const juce::String& label,
                             juce::Rectangle<int> vr, const juce::String& value)
        {
            g.setColour(t.textSecondary);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(label, lr, juce::Justification::centred, false);
            g.setColour(t.textPrimary);
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText(value, vr, juce::Justification::centred, false);
        };

        drawRow1(VoxlineLayout::compLabelBounds, "COMP",
                 VoxlineLayout::compValueBounds, "42%");
        drawRow1(VoxlineLayout::thresholdLabelBounds, "THRESHOLD",
                 VoxlineLayout::thresholdValueBounds, "-18.0 dB");

        // -- Row 2 labels & values (10px label, 11px value) --
        auto drawRow2 = [&](juce::Rectangle<int> lr, const juce::String& label,
                             juce::Rectangle<int> vr, const juce::String& value)
        {
            g.setColour(t.textSecondary);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(label, lr, juce::Justification::centred, false);
            g.setColour(t.textPrimary);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(value, vr, juce::Justification::centred, false);
        };

        drawRow2(VoxlineLayout::ratioLabelBounds, "RATIO",
                 VoxlineLayout::ratioValueBounds, "3.0:1");
        drawRow2(VoxlineLayout::attackLabelBounds, "ATTACK",
                 VoxlineLayout::attackValueBounds, "15 ms");
        drawRow2(VoxlineLayout::releaseLabelBounds, "RELEASE",
                 VoxlineLayout::releaseValueBounds, "80 ms");
        drawRow2(VoxlineLayout::driveLabelBounds, "DRIVE",
                 VoxlineLayout::driveValueBounds, "18%");
    }

    // Space / Monitor panel
    {
        const auto& t = VoxlineTheme::get(currentThemeIndex);

        // AMOUNT label
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("AMOUNT", VoxlineLayout::spaceAmountLabelBounds, juce::Justification::centredLeft, false);

        // Three knobs labels & values
        auto drawSpaceKnob = [&](juce::Rectangle<int> lr, const juce::String& label,
                                  juce::Rectangle<int> vr, const juce::String& value)
        {
            g.setColour(t.textSecondary);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(label, lr, juce::Justification::centred, false);
            g.setColour(t.textPrimary);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(value, vr, juce::Justification::centred, false);
        };

        drawSpaceKnob(VoxlineLayout::spacePreDelayLabelBounds, "PRE-DELAY",
                      VoxlineLayout::spacePreDelayValueBounds, "15 ms");
        drawSpaceKnob(VoxlineLayout::spaceHpfLabelBounds, "HPF",
                      VoxlineLayout::spaceHpfValueBounds, "200 Hz");
        drawSpaceKnob(VoxlineLayout::spaceLpfLabelBounds, "LPF",
                      VoxlineLayout::spaceLpfValueBounds, "8.0 kHz");
    }
#endif
}

void VoxlineAudioProcessorEditor::paintNewInterface(juce::Graphics& g)
{
    const auto& t = VoxlineTheme::get(currentThemeIndex);
    const auto accent = juce::Colour(0xffF06A3D);
    const auto surface = juce::Colour(0xff151616);
    const auto raised = juce::Colour(0xff1B1C1C);
    const auto recessed = juce::Colour(0xff0F1010);
    const auto hairline = juce::Colour(0xff373735);
    juce::ColourGradient background(juce::Colour(0xff151616), 0.0f, 0.0f,
                                    juce::Colour(0xff090A0A), 0.0f, 940.0f, false);
    g.setGradientFill(background);
    g.fillAll();

    auto panel = [&](juce::Rectangle<int> r, float radius = 9.0f)
    {
        g.setColour(juce::Colours::black.withAlpha(0.48f));
        g.fillRoundedRectangle(r.translated(0, 3).toFloat(), radius);
        juce::ColourGradient fill(raised, static_cast<float>(r.getX()), static_cast<float>(r.getY()),
                                  surface, static_cast<float>(r.getRight()), static_cast<float>(r.getBottom()), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(r.toFloat(), radius);
        g.setColour(hairline.withAlpha(0.9f));
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), radius, 1.0f);
    };

    panel({8, 8, 1064, 60}, 7.0f);
    panel({18, 78, 1044, 258}, 10.0f);
    panel({18, 346, 1044, 166}, 9.0f);
    if (advancedOpen)
        panel({18, 527, 1044, 400}, 10.0f);

    auto title = [&](const juce::String& text, juce::Rectangle<int> area,
                     juce::Justification justification = juce::Justification::centredLeft)
    {
        g.setColour(t.textPrimary);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)).withExtraKerningFactor(0.08f));
        g.drawText(text, area, justification, false);
    };
    auto caption = [&](const juce::String& text, juce::Rectangle<int> area,
                       juce::Justification justification = juce::Justification::centred)
    {
        g.setColour(t.textSecondary);
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)).withExtraKerningFactor(0.08f));
        g.drawText(text, area, justification, false);
    };
    auto value = [&](const juce::String& text, juce::Rectangle<int> area, float size = 14.0f)
    {
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(size, juce::Font::bold));
        g.drawText(text, area, juce::Justification::centred, false);
    };
    auto parameterText = [&](const char* id)
    {
        if (auto* value = audioProcessor.getAPVTS().getRawParameterValue(id))
        {
            const auto plain = value->load();
            if (juce::String(id) == VoxlineParameterIDs::spaceSlapTime)
                return juce::String(plain, 0) + " ms";
            if (juce::String(id) == VoxlineParameterIDs::spacePreDelay)
                return juce::String(plain, 0) + " ms";
            if (juce::String(id) == VoxlineParameterIDs::spaceTone)
                return (plain > 0.0f ? "+" : "") + juce::String(plain, 0);
            if (juce::String(id) == VoxlineParameterIDs::spaceDecay)
                return juce::String(plain, 2) + " s";
        }
        if (auto* p = audioProcessor.getAPVTS().getParameter(id))
            return p->getCurrentValueAsText();
        return juce::String("--");
    };

    g.setColour(t.textPrimary);
    g.setFont(juce::Font(juce::FontOptions(25.0f, juce::Font::bold)).withExtraKerningFactor(0.06f));
    g.drawText("VOXLINE", juce::Rectangle<int>{25, 16, 180, 28}, juce::Justification::centredLeft, false);
    g.setColour(t.textMuted);
    g.setFont(juce::Font(juce::FontOptions(7.5f)).withExtraKerningFactor(0.06f));
    g.drawText("COMPLETE VOCAL CHANNEL", juce::Rectangle<int>{27, 43, 190, 12}, juce::Justification::centredLeft, false);
    g.setColour(presetSession.isEdited() ? accent : t.textMuted);
    g.fillEllipse(521.0f, 32.0f, 6.0f, 6.0f);

    g.setColour(hairline.withAlpha(0.72f));
    g.drawVerticalLine(334, 94.0f, 321.0f);
    g.drawVerticalLine(746, 94.0f, 321.0f);
    title("INPUT", {40, 91, 150, 21});
    title("POLISH", {334, 91, 412, 21}, juce::Justification::centred);
    title("OUTPUT", {770, 91, 150, 21});

    const auto inputFrame = audioProcessor.getInputMeterFrame();
    const auto outputFrame = audioProcessor.getOutputMeterFrame();
    const auto inputPeak = inputFrame.channels[0].peakDbfs;
    const auto inputRms = inputFrame.channels[0].rmsDbfs;
    const auto meterFill = [&](juce::Rectangle<int> area, float rmsDbfs, float peakDbfs)
    {
        const auto normalise = [](float db) { return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f); };
        g.setColour(recessed);
        g.fillRoundedRectangle(area.toFloat(), 4.0f);
        const auto rmsWidth = area.getWidth() * normalise(rmsDbfs);
        g.setColour(rmsDbfs >= -6.0f ? juce::Colour(0xffD96A3D) : juce::Colour(0xff82A866));
        g.fillRoundedRectangle(area.withWidth(juce::roundToInt(rmsWidth)).toFloat(), 4.0f);
        const auto peakX = area.getX() + juce::roundToInt(area.getWidth() * normalise(peakDbfs));
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawVerticalLine(peakX, static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
        const auto referenceX = area.getX() + juce::roundToInt(area.getWidth() * normalise(-18.0f));
        g.setColour(t.textMuted.withAlpha(0.75f));
        g.drawVerticalLine(referenceX, static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
    };

    caption("GAIN", {61, 128, 100, 17});
    value(parameterText(VoxlineParameterIDs::inputGain), {57, 275, 108, 21}, 14.0f);
    caption("INPUT METER / -18 dBFS", {178, 137, 145, 16});
    meterFill({190, 188, 118, 12}, inputRms, inputPeak);
    caption("PEAK", {183, 214, 56, 15});
    value(juce::String(inputPeak, 1) + " dBFS", {180, 232, 60, 18}, 10.5f);
    caption("RMS", {252, 214, 56, 15});
    value(juce::String(inputRms, 1) + " dBFS", {248, 232, 65, 18}, 10.5f);

    value(parameterText(VoxlineParameterIDs::polish), {465, 273, 150, 36}, 29.0f);
    const auto polish = audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::polish)->load();
    const auto status = polish <= 33.0f ? "NATURAL" : (polish <= 72.0f ? "POLISHED" : "PUSHED");
    const auto statusBounds = juce::Rectangle<int>{493, 307, 94, 20};
    g.setColour(accent.withAlpha(0.10f));
    g.fillRoundedRectangle(statusBounds.toFloat(), 12.0f);
    g.setColour(accent);
    g.drawRoundedRectangle(statusBounds.toFloat().reduced(0.5f), 12.0f, 1.0f);
    g.setColour(accent);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText(status, statusBounds, juce::Justification::centred, false);

    const auto outRmsDb = outputFrame.channels[0].rmsDbfs;
    caption("TRUE PEAK", {764, 145, 80, 16});
    value(juce::String(outputFrame.channels[0].truePeakDbtp, 1) + " dBTP", {755, 166, 112, 28}, 15.0f);
    caption("RMS", {764, 229, 70, 16});
    value(juce::String(outRmsDb, 1) + " dBFS", {755, 250, 112, 28}, 15.0f);

    auto segmentedMeter = [&](juce::Rectangle<int> r, float level, bool gr)
    {
        constexpr int segments = 15;
        const auto lit = juce::jlimit(0, segments, juce::roundToInt(level * segments));
        for (int i = 0; i < segments; ++i)
        {
            const auto y = r.getBottom() - (i + 1) * (r.getHeight() / segments);
            auto c = gr ? accent
                        : (i < 8 ? juce::Colour(0xff66864B)
                                 : (i < 12 ? juce::Colour(0xffB79B45) : juce::Colour(0xffD96A3D)));
            g.setColour(i < lit ? c : c.withAlpha(0.12f));
            g.fillRoundedRectangle(static_cast<float>(r.getX()), static_cast<float>(y + 2),
                                   static_cast<float>(r.getWidth()), 6.0f, 2.0f);
        }
    };
    caption("L", {875, 125, 36, 16});
    caption("R", {920, 125, 36, 16});
    segmentedMeter({882, 148, 15, 156}, juce::jlimit(0.0f, 1.0f, (outRmsDb + 60.0f) / 60.0f), false);
    segmentedMeter({927, 148, 15, 156}, juce::jlimit(0.0f, 1.0f, (outputFrame.channels[1].rmsDbfs + 60.0f) / 60.0f), false);
    caption("OUTPUT GAIN", {963, 139, 82, 17});
    value(parameterText(VoxlineParameterIDs::outputGain), {963, 285, 88, 22}, 14.0f);
    if (outputFrame.clipHeld || audioProcessor.wasOutputSafetyActive())
    {
        g.setColour(juce::Colour(0xffD96A3D));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(outputFrame.clipHeld ? "CLIP" : "SOFT CLIP", juce::Rectangle<int>{960, 119, 86, 14}, juce::Justification::centred, false);
    }

    struct MainControl { const char* label; const char* id; int x; };
    const MainControl controls[] = {
        {"BODY", VoxlineParameterIDs::body, 42}, {"PRESENCE", VoxlineParameterIDs::clarity, 190},
        {"AIR", VoxlineParameterIDs::air, 338}, {"DE-ESS", VoxlineParameterIDs::smooth, 486},
        {"COMP", VoxlineParameterIDs::comp, 634}, {"DRIVE", VoxlineParameterIDs::drive, 782},
        {"SPACE", VoxlineParameterIDs::spaceAmount, 930},
    };
    for (const auto& c : controls)
    {
        const auto amount = audioProcessor.getAPVTS().getRawParameterValue(c.id)->load();
        g.setColour(std::abs(amount) > 0.001f ? accent : t.textMuted.withAlpha(0.55f));
        g.fillEllipse(static_cast<float>(c.x + 43), 358.0f, 7.0f, 7.0f);
        caption(c.label, {c.x, 370, 94, 18});
        if (c.x < 470)
            value(parameterText(c.id), {c.x, 478, 94, 22}, 13.0f);
    }

    const auto deEssReduction = audioProcessor.deEssReduction.load();
    caption("FOCUS " + parameterText(VoxlineParameterIDs::deEssFreq), {486, 478, 94, 15});
    value((deEssReduction > 0.5f ? "S ACTIVE " : "") + juce::String(deEssReduction, 1) + " dB GR", {486, 492, 94, 16}, 9.5f);
    const auto compAmount = audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::comp)->load();
    caption(compAmount < 34.0f ? "LIGHT" : (compAmount < 73.0f ? "CONTROLLED" : "FIRM"), {634, 478, 94, 15});
    value(juce::String(audioProcessor.getGainReductionDb(), 1) + " dB GR", {634, 492, 94, 16}, 10.5f);
    const auto reductionBar = [&](int x, float reduction, juce::Colour colour)
    {
        const auto area = juce::Rectangle<int>{x, 466, 94, 3};
        g.setColour(recessed);
        g.fillRoundedRectangle(area.toFloat(), 1.5f);
        g.setColour(colour);
        g.fillRoundedRectangle(area.withWidth(juce::roundToInt(area.getWidth()
            * juce::jlimit(0.0f, 1.0f, reduction / 12.0f))).toFloat(), 1.5f);
    };
    reductionBar(486, deEssReduction, juce::Colour(0xffD96A3D));
    reductionBar(634, audioProcessor.getGainReductionDb(), juce::Colour(0xffA58AEF));
    const auto driveAmount = audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::drive)->load();
    caption(driveAmount < 1.0f ? "NEUTRAL" : (driveAmount <= 40.0f ? "WARM" : (driveAmount <= 75.0f ? "GRIT" : "EDGE")), {782, 478, 94, 15});
    value(juce::String(driveAmount, 0) + "%", {782, 492, 94, 16}, 10.5f);
    const auto spaceMode = juce::roundToInt(audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::spaceMode)->load());
    caption(spaceModeName(spaceMode), {930, 478, 94, 15});
    value(parameterText(VoxlineParameterIDs::spaceAmount), {930, 492, 94, 16}, 10.5f);

    g.setColour(t.textMuted.withAlpha(0.62f));
    g.setFont(juce::FontOptions(8.0f));
    for (int x : {42, 190, 338})
        g.drawText("-6          0          +6", juce::Rectangle<int>{x, 465, 94, 14},
                   juce::Justification::centred, false);

    g.setColour(t.textMuted.withAlpha(0.35f));
    for (int x : {174, 322, 470, 618, 766, 914})
        g.drawVerticalLine(x, 361.0f, 497.0f);

    if (! advancedOpen)
    {
        g.setColour(t.textMuted.withAlpha(0.55f));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText("VOXLINE 2.0  |  SADTONY", juce::Rectangle<int>{15, 695, 1050, 16}, juce::Justification::centred, false);
        return;
    }

    g.setColour(hairline.withAlpha(0.72f));
    g.drawHorizontalLine(575, 34.0f, 1046.0f);
    const int activeTabX = advancedSection == AdvancedSection::eq ? 252
                         : advancedSection == AdvancedSection::comp ? 372
                         : advancedSection == AdvancedSection::deEss ? 492
                         : advancedSection == AdvancedSection::drive ? 612 : 732;
    g.setColour(accent);
    g.fillRoundedRectangle(static_cast<float>(activeTabX + 20), 571.0f, 72.0f, 3.0f, 1.5f);

    if (advancedSection == AdvancedSection::eq)
    {
        const auto graph = getEqGraphBounds();
        g.setColour(recessed);
        g.fillRoundedRectangle(graph, 6.0f);
        g.setColour(hairline.withAlpha(0.48f));
        for (float hz : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f,
                          2000.0f, 5000.0f, 10000.0f, 20000.0f })
        {
            const auto norm = std::log(hz / 20.0f) / std::log(1000.0f);
            const auto x = graph.getX() + norm * graph.getWidth();
            g.drawVerticalLine(juce::roundToInt(x), graph.getY(), graph.getBottom());
        }
        constexpr auto displayDb = 12.0f;
        for (float db : {-12.0f, -6.0f, 0.0f, 6.0f, 12.0f})
        {
            const auto y = graph.getCentreY() - db / (displayDb * 2.0f) * graph.getHeight();
            g.setColour(db == 0.0f ? t.textMuted.withAlpha(0.65f) : hairline.withAlpha(0.36f));
            g.drawHorizontalLine(juce::roundToInt(y), graph.getX(), graph.getRight());
            caption((db > 0.0f ? "+" : "") + juce::String(db, 0),
                    {42, juce::roundToInt(y) - 7, 30, 14}, juce::Justification::centredLeft);
        }

        juce::Path spectrumPath;
        for (size_t i = 0; i < spectrumDisplay.size(); ++i)
        {
            const auto norm = static_cast<float>(i) / static_cast<float>(spectrumDisplay.size() - 1);
            const auto y = juce::jmap(spectrumDisplay[i], -72.0f, 0.0f,
                                      graph.getBottom(), graph.getY() + 8.0f);
            const auto point = juce::Point<float>{graph.getX() + norm * graph.getWidth(), y};
            if (i == 0)
                spectrumPath.startNewSubPath(point);
            else
                spectrumPath.lineTo(point);
        }
        auto spectrumFill = spectrumPath;
        spectrumFill.lineTo(graph.getRight(), graph.getBottom());
        spectrumFill.lineTo(graph.getX(), graph.getBottom());
        spectrumFill.closeSubPath();
        g.setColour(juce::Colour(0xffB9B2A8).withAlpha(0.08f));
        g.fillPath(spectrumFill);
        g.setColour(juce::Colour(0xffB9B2A8).withAlpha(0.24f));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.0f));

        const auto raw = [&](const char* id, float fallback)
        {
            if (auto* parameter = audioProcessor.getAPVTS().getRawParameterValue(id))
                return parameter->load();
            return fallback;
        };
        const auto sampleRate = juce::jmax(1.0, audioProcessor.getSampleRate());
        const auto filterMagnitude = [sampleRate](const juce::IIRCoefficients& coefficients, double hz)
        {
            const auto phase = -juce::MathConstants<double>::twoPi * hz / sampleRate;
            const auto z1 = std::polar(1.0, phase);
            const auto z2 = std::polar(1.0, phase * 2.0);
            const auto* c = coefficients.coefficients;
            const auto numerator = static_cast<double>(c[0])
                                 + static_cast<double>(c[1]) * z1
                                 + static_cast<double>(c[2]) * z2;
            const auto denominator = 1.0 + static_cast<double>(c[3]) * z1
                                          + static_cast<double>(c[4]) * z2;
            return std::abs(numerator / denominator);
        };
        const auto responseDb = [&](double hz)
        {
            auto magnitude = 1.0;
            if (raw(VoxlineParameterIDs::eqEnabled, 1.0f) < 0.5f)
                return 0.0;
            const auto hpf = juce::IIRCoefficients::makeHighPass(sampleRate, raw(VoxlineParameterIDs::hpfFreq, 80.0f));
            const auto lpf = juce::IIRCoefficients::makeLowPass(sampleRate, raw(VoxlineParameterIDs::lpfFreq, 18000.0f));
            if (raw(VoxlineParameterIDs::hpfEnabled, 0.0f) >= 0.5f)
                for (int i = 0; i <= juce::roundToInt(raw(VoxlineParameterIDs::hpfSlope, 1.0f)); ++i)
                    magnitude *= filterMagnitude(hpf, hz);
            if (raw(VoxlineParameterIDs::lpfEnabled, 0.0f) >= 0.5f)
                for (int i = 0; i <= juce::roundToInt(raw(VoxlineParameterIDs::lpfSlope, 0.0f)); ++i)
                    magnitude *= filterMagnitude(lpf, hz);
            const auto filters = std::array<juce::IIRCoefficients, 4> {
                juce::IIRCoefficients::makePeakFilter(sampleRate, raw(VoxlineParameterIDs::lowFreq, 160.0f),
                    raw(VoxlineParameterIDs::lowQ, 0.8f), juce::Decibels::decibelsToGain(raw(VoxlineParameterIDs::body, 0.0f))),
                juce::IIRCoefficients::makePeakFilter(sampleRate, raw(VoxlineParameterIDs::mudFreq, 350.0f),
                    raw(VoxlineParameterIDs::mudQ, 1.1f), juce::Decibels::decibelsToGain(raw(VoxlineParameterIDs::mudGain, -2.0f))),
                juce::IIRCoefficients::makePeakFilter(sampleRate, raw(VoxlineParameterIDs::presFreq, 2500.0f),
                    raw(VoxlineParameterIDs::presQ, 1.0f), juce::Decibels::decibelsToGain(raw(VoxlineParameterIDs::clarity, 0.0f))),
                juce::IIRCoefficients::makeHighShelf(sampleRate, raw(VoxlineParameterIDs::airFreq, 10000.0f),
                    raw(VoxlineParameterIDs::airQ, 0.7f), juce::Decibels::decibelsToGain(raw(VoxlineParameterIDs::air, 0.0f)))
            };
            const bool enabled[] = {
                raw(VoxlineParameterIDs::lowEnabled, 1.0f) >= 0.5f,
                raw(VoxlineParameterIDs::mudEnabled, 0.0f) >= 0.5f,
                raw(VoxlineParameterIDs::presEnabled, 1.0f) >= 0.5f,
                raw(VoxlineParameterIDs::airEnabled, 1.0f) >= 0.5f
            };
            for (size_t index = 0; index < filters.size(); ++index)
                if (enabled[index])
                    magnitude *= filterMagnitude(filters[index], hz);
            return juce::Decibels::gainToDecibels(magnitude, -60.0);
        };

        juce::Path curve;
        constexpr int responseSteps = 360;
        for (int i = 0; i <= responseSteps; ++i)
        {
            const auto norm = static_cast<float>(i) / static_cast<float>(responseSteps);
            const auto hz = 20.0f * std::pow(1000.0f, norm);
            const auto db = juce::jlimit(-displayDb, displayDb, static_cast<float>(responseDb(hz)));
            const auto point = juce::Point<float>{graph.getX() + norm * graph.getWidth(),
                graph.getCentreY() - db / (displayDb * 2.0f) * graph.getHeight()};
            if (i == 0)
                curve.startNewSubPath(point);
            else
                curve.lineTo(point);
        }
        g.setColour(accent.withAlpha(0.13f));
        auto fill = curve;
        fill.lineTo(graph.getRight(), graph.getBottom());
        fill.lineTo(graph.getX(), graph.getBottom());
        fill.closeSubPath();
        g.fillPath(fill);
        g.setColour(accent);
        g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
        for (int i = 0; i < 6; ++i)
        {
            const auto point = getEqNodePosition(i);
            const auto active = i == selectedEqBand || i == hoveredEqBand;
            const auto diameter = active ? 18.0f : 14.0f;
            const auto nodeBounds = juce::Rectangle<float>(diameter, diameter).withCentre(point);
            g.setColour(active ? accent : juce::Colour(0xff242424));
            g.fillEllipse(nodeBounds);
            g.setColour(active ? accent.brighter(0.15f) : t.textPrimary);
            g.drawEllipse(nodeBounds.reduced(0.5f), 1.2f);
            g.setFont(juce::FontOptions(active ? 9.5f : 8.5f, juce::Font::bold));
            g.drawText(juce::String(i + 1), nodeBounds.getSmallestIntegerContainer(),
                       juce::Justification::centred, false);
            if (active)
            {
                g.setColour(accent.withAlpha(0.28f));
                g.drawEllipse(point.x - 12.0f, point.y - 12.0f, 24.0f, 24.0f, 1.5f);
            }
        }
        const auto frequencyX = [&](float hz)
        {
            return juce::roundToInt(graph.getX()
                + std::log(hz / 20.0f) / std::log(1000.0f) * graph.getWidth());
        };
        for (const auto& mark : std::initializer_list<std::pair<float, const char*>>{
                 {20.0f, "20"}, {50.0f, "50"}, {100.0f, "100"}, {200.0f, "200"},
                 {500.0f, "500"}, {1000.0f, "1k"}, {2000.0f, "2k"},
                 {5000.0f, "5k"}, {10000.0f, "10k"}, {20000.0f, "20k"}})
            caption(mark.second, {frequencyX(mark.first) - 19, 777, 38, 12});

        caption("FILTER TYPE", {35, 802, 185, 14}, juce::Justification::centredLeft);
        caption("FREQ", {588, 802, 100, 14});
        caption(selectedEqBand == 0 || selectedEqBand == 5 ? "SLOPE" : "GAIN", {718, 802, 100, 14});
        caption("Q", {848, 802, 100, 14});
        g.setColour(t.textMuted);
        g.setFont(juce::FontOptions(8.5f));
        g.drawText("Drag: frequency + gain   |   Wheel: Q/slope   |   Shift: fine   |   Double-click: reset",
                   juce::Rectangle<int>{36, 902, 810, 14}, juce::Justification::centredLeft, false);
        caption("LATENCY", {878, 901, 60, 14});
        value("0 samples", {937, 901, 75, 14}, 9.0f);
    }
    else
    {
        const char* labels[5] = {};
        const char* ids[5] = {};
        int count = 0;
        if (advancedSection == AdvancedSection::comp)
        {
            labels[0] = "SENSITIVITY"; ids[0] = VoxlineParameterIDs::compSensitivity;
            labels[1] = "RATIO"; ids[1] = VoxlineParameterIDs::compRatio;
            labels[2] = "ATTACK"; ids[2] = VoxlineParameterIDs::compAttack;
            labels[3] = "RELEASE"; ids[3] = VoxlineParameterIDs::compRelease;
            labels[4] = "MIX"; ids[4] = VoxlineParameterIDs::compMix;
            count = 5;
        }
        else if (advancedSection == AdvancedSection::deEss)
        {
            labels[0] = "FREQUENCY"; ids[0] = VoxlineParameterIDs::deEssFreq;
            labels[1] = "SENSITIVITY"; ids[1] = VoxlineParameterIDs::deEssThreshold;
            labels[2] = "RANGE"; ids[2] = VoxlineParameterIDs::deEssRange;
            count = 3;
            caption("MODE", {810, 620, 150, 16});
            value(juce::String(audioProcessor.deEssReduction.load(), 1) + " dB REDUCTION",
                  {790, 885, 190, 18}, 11.5f);
        }
        else if (advancedSection == AdvancedSection::drive)
        {
            labels[0] = "TONE"; ids[0] = VoxlineParameterIDs::driveTone;
            labels[1] = "MIX"; ids[1] = VoxlineParameterIDs::driveMix;
            count = 2;
            caption("CHARACTER", {790, 620, 170, 16});
        }
        else
        {
            const auto* modeValue = audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::spaceMode);
            const auto mode = juce::roundToInt(modeValue != nullptr ? modeValue->load() : 0.0f);
            if (mode == 4)
            {
                labels[0] = "DELAY"; ids[0] = VoxlineParameterIDs::spacePreDelay;
                labels[1] = "SPREAD"; ids[1] = VoxlineParameterIDs::spaceWidth;
                labels[2] = "TONE"; ids[2] = VoxlineParameterIDs::spaceTone;
                count = 3;
                caption("MONO SAFETY", {720, 780, 150, 16});
                value(audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::spaceMonoSafety)->load() >= 0.5f
                          ? "ON" : "OFF", {720, 885, 150, 16}, 11.5f);
            }
            else
            {
                if (mode == 3)
                {
                    labels[0] = "TIME"; ids[0] = VoxlineParameterIDs::spaceSlapTime;
                    labels[1] = "WIDTH"; ids[1] = VoxlineParameterIDs::spaceWidth;
                    labels[2] = "TONE"; ids[2] = VoxlineParameterIDs::spaceTone;
                    labels[3] = "FEEDBACK"; ids[3] = VoxlineParameterIDs::spaceFeedback;
                    count = 4;
                }
                else
                {
                    labels[0] = "SIZE"; ids[0] = VoxlineParameterIDs::spaceSize;
                    labels[1] = "PRE-DELAY"; ids[1] = VoxlineParameterIDs::spacePreDelay;
                    labels[2] = "WIDTH"; ids[2] = VoxlineParameterIDs::spaceWidth;
                    labels[3] = "TONE"; ids[3] = VoxlineParameterIDs::spaceTone;
                    labels[4] = "DECAY"; ids[4] = VoxlineParameterIDs::spaceDecay;
                    count = 5;
                }
                caption("DUCKING", {900, 620, 100, 16});
                value(parameterText(VoxlineParameterIDs::spaceDucking), {900, 885, 100, 16}, 11.5f);
            }
            caption("SPACE TYPE", {720, 620, 150, 16});
        }

        const int compXs[] = {70, 260, 450, 640, 830};
        const int deEssXs[] = {100, 320, 540};
        const int driveXs[] = {155, 390};
        for (int i = 0; i < count; ++i)
        {
            const int spaceXs[] = {45, 175, 305, 435, 565};
            const int widthXs[] = {45, 175, 435};
            const int slapXs[] = {45, 305, 435, 565};
            const auto x = advancedSection == AdvancedSection::comp ? compXs[i]
                         : (advancedSection == AdvancedSection::deEss ? deEssXs[i]
                         : (advancedSection == AdvancedSection::drive ? driveXs[i]
                         : (advancedSection == AdvancedSection::space
                            && juce::roundToInt(audioProcessor.getAPVTS()
                                .getRawParameterValue(VoxlineParameterIDs::spaceMode)->load()) == 4
                                ? widthXs[i]
                                : (advancedSection == AdvancedSection::space
                                   && juce::roundToInt(audioProcessor.getAPVTS()
                                       .getRawParameterValue(VoxlineParameterIDs::spaceMode)->load()) == 3
                                   ? slapXs[i] : spaceXs[i]))));
            caption(labels[i], {x, 620, 100, 16});
            value(parameterText(ids[i]), {x, 885, 100, 16}, 11.5f);
        }
    }
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::resized()
{
    presetPreviousButton.setBounds({212, 20, 38, 36});
    presetNextButton.setBounds({252, 20, 38, 36});
    presetDropdown.setBounds({294, 20, 238, 36});
    presetManageButton.setBounds({538, 20, 48, 36});
    savePresetButton.setBounds({594, 20, 90, 36});
    abButton.setBounds({704, 21, 58, 34});
    bypassButton.setBounds({899, 21, 100, 34});

    inputGainSlider.setBounds({68, 150, 88, 104});
    autoGainButton.setBounds({224, 154, 70, 28});
    polishSlider.setBounds({466, 116, 148, 148});
    outputGainSlider.setBounds({970, 183, 78, 88});
    outputClipClearButton.setBounds({950, 300, 96, 24});
    outputMeter.setBounds(VoxlineLayout::outMeterBounds);
    gainReductionMeter.setBounds(VoxlineLayout::grMeterBounds);

    bodySlider.setBounds({48, 393, 82, 72});
    claritySlider.setBounds({196, 393, 82, 72});
    airSlider.setBounds({344, 393, 82, 72});
    smoothSlider.setBounds({492, 393, 82, 72});
    compSlider.setBounds({640, 393, 82, 72});
    driveSlider.setBounds({788, 393, 82, 72});
    spaceSlider.setBounds({936, 393, 82, 72});
    advancedButton.setBounds({475, 511, 130, 28});

    advancedEqButton.setBounds({252, 537, 112, 34});
    advancedCompButton.setBounds({372, 537, 112, 34});
    advancedDeEssButton.setBounds({492, 537, 112, 34});
    advancedDriveButton.setBounds({612, 537, 112, 34});
    advancedSpaceButton.setBounds({732, 537, 112, 34});

    eqHpfButton.setBounds({36, 825, 83, 42});
    eqLowButton.setBounds({122, 825, 83, 42});
    eqMudButton.setBounds({208, 825, 83, 42});
    eqPresButton.setBounds({294, 825, 83, 42});
    eqAirButton.setBounds({380, 825, 83, 42});
    eqLpfButton.setBounds({466, 825, 83, 42});
    eqFreqKnob.setBounds({598, 819, 82, 72});
    eqGainKnob.setBounds({728, 819, 82, 72});
    eqQKnob.setBounds({858, 819, 82, 72});
    eqResetButton.setBounds({952, 824, 86, 31});
    eqRangeButton.setBounds({952, 861, 86, 31});
    eqBandEnableButton.setBounds({35, 875, 104, 28});
    eqBandSoloButton.setBounds({146, 875, 104, 28});
    eqOnButton.setBounds({1000, 585, 38, 27});

    thresholdKnob.setBounds({88, 684, 86, 96});
    ratioKnob.setBounds({278, 684, 86, 96});
    attackKnob.setBounds({468, 684, 86, 96});
    releaseKnob.setBounds({658, 684, 86, 96});
    compMixKnob.setBounds({848, 684, 86, 96});
    compMakeupKnob.setBounds({758, 804, 86, 90});
    compAutoMakeupButton.setBounds({866, 834, 142, 30});

    deEssFreqKnob.setBounds({118, 684, 86, 96});
    deEssThresholdKnob.setBounds({338, 684, 86, 96});
    deEssRangeKnob.setBounds({558, 684, 86, 96});
    deEssModeCombo.setBounds({810, 688, 150, 42});
    deEssListenButton.setBounds({810, 744, 150, 30});

    driveToneKnob.setBounds({173, 684, 86, 96});
    driveMixKnob.setBounds({408, 684, 86, 96});
    driveCharacterCombo.setBounds({790, 688, 170, 42});
    driveOutputTrimKnob.setBounds({642, 684, 86, 96});
    driveLevelMatchButton.setBounds({790, 744, 170, 30});

    spaceTimeKnob.setBounds({63, 684, 86, 96});
    spacePreDelayKnob.setBounds({193, 684, 86, 96});
    spaceWidthKnob.setBounds({323, 684, 86, 96});
    spaceToneKnob.setBounds({453, 684, 86, 96});
    spaceDecayKnob.setBounds({583, 684, 86, 96});
    spaceTypeCombo.setBounds({720, 688, 150, 42});
    spaceDuckingKnob.setBounds({918, 684, 86, 96});
    spaceMonoSafetyButton.setBounds({720, 744, 150, 30});

    updateAdvancedVisibility();
    return;

#if 0 // Retired V2 resize path. The active layout above is the only shipped UI.

    auto bounds = [this](const juce::String& key, juce::Rectangle<int> fallback)
    {
        const auto r = layout.getBounds(key);
        return r.isEmpty() ? fallback : r;
    };

    logoLabel.setBounds(bounds("logo", VoxlineLayout::logoBounds));
    subtitleLabel.setBounds(bounds("subtitle", VoxlineLayout::subtitleBounds));

    // === V2 Top Bar ===
    presetDropdown.setBounds(bounds("presetDropdown", VoxlineLayout::presetDropdownBounds));
    abButton.setBounds(bounds("abButton", VoxlineLayout::abButtonBounds));
    listenButton.setBounds(bounds("listenButton", VoxlineLayout::listenUtilityBounds));
    bypassButton.setBounds(bounds("bypassToggle", VoxlineLayout::bypassToggleBounds));

    // === V2 Input / Clean Panel ===
    inputTitleLabel.setBounds(bounds("inputTitle", VoxlineLayout::inputTitleBounds));
    inputGainSlider.setBounds(bounds("inputGainKnob", VoxlineLayout::inputGainKnobBounds));
    lowCutKnob.setBounds(bounds("lowCutKnob", VoxlineLayout::lowCutKnobBounds));
    cleanKnob.setBounds(bounds("cleanKnob", VoxlineLayout::cleanKnobBounds));
    deEssKnob.setBounds(bounds("deEssKnob", VoxlineLayout::deEssKnobBounds));
    autoGainButton.setBounds(bounds("autoGainToggle", VoxlineLayout::autoGainToggleBounds));

    // === V2 POLISH Hero Panel ===
    polishTitleLabel.setBounds(bounds("polishTitle", VoxlineLayout::polishTitleBounds));
    polishSlider.setBounds(bounds("polishKnob", VoxlineLayout::polishSliderBounds));

    // === V2 Output Panel ===
    outputTitleLabel.setBounds(bounds("outputTitle", VoxlineLayout::outputTitleBounds));
    outputMeter.setBounds(bounds("outMeter", VoxlineLayout::outMeterBounds));
    gainReductionMeter.setBounds(bounds("grMeter", VoxlineLayout::grMeterBounds));
    outputGainSlider.setBounds(bounds("outputGainKnob", VoxlineLayout::outputGainKnobBounds));

    // === V2 Vocal EQ Panel (lower row) ===
    // EQ band label buttons (HPF/LOW/MUD/PRES/AIR/LPF)
    toneTitleLabel.setBounds(bounds("eqTitle", VoxlineLayout::eqTitleBounds));
    eqOnButton.setBounds(bounds("eqOnToggle", VoxlineLayout::eqOnToggleBounds));
    eqFreqKnob.setBounds(bounds("eqFreqKnob", VoxlineLayout::eqFreqKnobBounds));
    eqGainKnob.setBounds(bounds("eqGainKnob", VoxlineLayout::eqQKnobBounds));

    eqHpfButton.setBounds(bounds("eqHpfBtn", VoxlineLayout::eqHpfBounds));
    eqLowButton.setBounds(bounds("eqLowBtn", VoxlineLayout::eqLowBounds));
    eqMudButton.setBounds(bounds("eqMudBtn", VoxlineLayout::eqMudBounds));
    eqPresButton.setBounds(bounds("eqPresBtn", VoxlineLayout::eqPresBounds));
    eqAirButton.setBounds(bounds("eqAirBtn", VoxlineLayout::eqAirBounds));
    eqLpfButton.setBounds(bounds("eqLpfBtn", VoxlineLayout::eqLpfBounds));

    // === V2 Dynamics / Color Panel (lower row) ===
    meterNamesLabel.setBounds(bounds("dynamicsTitle", VoxlineLayout::dynamicsTitleBounds));
    compSlider.setBounds(bounds("compKnob", VoxlineLayout::compKnobBounds));
    thresholdKnob.setBounds(bounds("thresholdKnob", VoxlineLayout::thresholdKnobBounds));
    driveSlider.setBounds(bounds("driveKnob", VoxlineLayout::driveKnobBounds));
    ratioKnob.setBounds(bounds("ratioKnob", VoxlineLayout::ratioKnobBounds));
    attackKnob.setBounds(bounds("attackKnob", VoxlineLayout::attackKnobBounds));
    releaseKnob.setBounds(bounds("releaseKnob", VoxlineLayout::releaseKnobBounds));

    // === V2 Space / Monitor Panel (lower row) ===
    spaceTitleLabel.setBounds(bounds("spaceTitle", VoxlineLayout::spaceTitleBounds));
    spaceTypeCombo.setBounds(bounds("spaceTypeCombo", VoxlineLayout::spaceTypeBounds));
    spaceSlider.setBounds(bounds("spaceSlider", VoxlineLayout::spaceSliderBounds));
    spaceAmountLabel.setBounds(bounds("spaceAmountLabel", VoxlineLayout::spaceAmountLabelBounds));
    preDelayKnob.setBounds(bounds("spacePreDelayKnob", VoxlineLayout::spacePreDelayKnobBounds));
    spaceHpfKnob.setBounds(bounds("spaceHpfKnob", VoxlineLayout::spaceHpfKnobBounds));
    spaceLpfKnob.setBounds(bounds("spaceLpfKnob", VoxlineLayout::spaceLpfKnobBounds));
    monitorLabel.setBounds(VoxlineLayout::monitorTitleBounds);
    monitorAbBtn.setBounds(VoxlineLayout::monitorAbBounds);
    monitorListenBtn.setBounds(VoxlineLayout::monitorListenBounds);
    monitorBypassBtn.setBounds(VoxlineLayout::monitorBypassBounds);

    // === Footer ===
    footerLabel.setBounds(bounds("footer", VoxlineLayout::footerBounds));

    // === Placeholder labels ===
    thresholdLabel.setBounds(VoxlineLayout::thresholdLabelBounds);
    preDelayLabel.setBounds(VoxlineLayout::spacePreDelayLabelBounds);
    spaceHpfLabel.setBounds(VoxlineLayout::spaceHpfLabelBounds);
    spaceLpfLabel.setBounds(VoxlineLayout::spaceLpfLabelBounds);
#endif
}

juce::Rectangle<float> VoxlineAudioProcessorEditor::getEqGraphBounds() const
{
    return {36.0f, 616.0f, 1002.0f, 161.0f};
}

juce::Point<float> VoxlineAudioProcessorEditor::getEqNodePosition(int band) const
{
    const auto graph = getEqGraphBounds();
    auto& apvts = audioProcessor.getAPVTS();
    const auto raw = [&](const char* id, float fallback)
    {
        if (auto* value = apvts.getRawParameterValue(id))
            return value->load();
        return fallback;
    };
    const auto frequency = raw(kEqFreqIDs[band], 1000.0f);
    const auto xNorm = std::log(juce::jlimit(20.0f, 20000.0f, frequency) / 20.0f)
                     / std::log(1000.0f);
    auto gain = 0.0f;
    if (band > 0 && band < 5)
        gain = raw(kEqGainIDs[band], 0.0f);
    const auto range = eqShows24dB ? 48.0f : 24.0f;
    const auto yNorm = juce::jlimit(0.0f, 1.0f, 0.5f - gain / range);
    return {graph.getX() + xNorm * graph.getWidth(),
            graph.getY() + yNorm * graph.getHeight()};
}

int VoxlineAudioProcessorEditor::findEqNode(juce::Point<float> position) const
{
    auto nearest = -1;
    auto distance = 18.0f;
    for (int band = 0; band < 6; ++band)
    {
        const auto d = position.getDistanceFrom(getEqNodePosition(band));
        if (d < distance)
        {
            nearest = band;
            distance = d;
        }
    }
    return nearest;
}

void VoxlineAudioProcessorEditor::updateEqNodeFromMouse(juce::Point<float> position)
{
    if (draggingEqBand < 0)
        return;
    const auto graph = getEqGraphBounds();
    const auto p = graph.getConstrainedPoint(position);
    const auto xNorm = juce::jlimit(0.0f, 1.0f, (p.x - graph.getX()) / graph.getWidth());
    const auto frequency = 20.0f * std::pow(1000.0f, xNorm);
    auto& apvts = audioProcessor.getAPVTS();
    if (auto* parameter = apvts.getParameter(kEqFreqIDs[draggingEqBand]))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(frequency));

    if (draggingEqBand > 0 && draggingEqBand < 5)
    {
        if (auto* parameter = apvts.getParameter(kEqGainIDs[draggingEqBand]))
        {
            const auto displayRange = eqShows24dB ? 48.0f : 24.0f;
            const auto requested = (0.5f - (p.y - graph.getY()) / graph.getHeight()) * displayRange;
            const auto range = parameter->getNormalisableRange();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(
                juce::jlimit(range.start, range.end, requested)));
        }
    }
    syncEQKnobsToSelectedBand();
    repaint();
}

void VoxlineAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (! advancedOpen || advancedSection != AdvancedSection::eq)
        return;
    draggingEqBand = findEqNode(event.position);
    if (draggingEqBand < 0)
        return;
    selectedEqBand = draggingEqBand;
    if (auto* parameter = audioProcessor.getAPVTS().getParameter(kEqFreqIDs[draggingEqBand]))
        parameter->beginChangeGesture();
    if (draggingEqBand > 0 && draggingEqBand < 5)
        if (auto* parameter = audioProcessor.getAPVTS().getParameter(kEqGainIDs[draggingEqBand]))
            parameter->beginChangeGesture();
    syncEQKnobsToSelectedBand();
    updateEqNodeFromMouse(event.position);
}

void VoxlineAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    updateEqNodeFromMouse(event.position);
}

void VoxlineAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    if (draggingEqBand < 0)
        return;
    if (auto* parameter = audioProcessor.getAPVTS().getParameter(kEqFreqIDs[draggingEqBand]))
        parameter->endChangeGesture();
    if (draggingEqBand > 0 && draggingEqBand < 5)
        if (auto* parameter = audioProcessor.getAPVTS().getParameter(kEqGainIDs[draggingEqBand]))
            parameter->endChangeGesture();
    draggingEqBand = -1;
}

void VoxlineAudioProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
    const auto next = advancedOpen && advancedSection == AdvancedSection::eq
                    ? findEqNode(event.position) : -1;
    if (next != hoveredEqBand)
    {
        hoveredEqBand = next;
        setMouseCursor(next >= 0 ? juce::MouseCursor::DraggingHandCursor
                                 : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void VoxlineAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto band = findEqNode(event.position);
    if (! advancedOpen || advancedSection != AdvancedSection::eq || band < 0)
        return;
    auto& apvts = audioProcessor.getAPVTS();
    for (auto* id : { kEqFreqIDs[band], kEqGainIDs[band], kEqQIDs[band] })
        if (id != nullptr)
            if (auto* parameter = apvts.getParameter(id))
                parameter->setValueNotifyingHost(parameter->getDefaultValue());
    selectedEqBand = band;
    syncEQKnobsToSelectedBand();
    repaint();
}

void VoxlineAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event,
                                                  const juce::MouseWheelDetails& wheel)
{
    if (! advancedOpen || advancedSection != AdvancedSection::eq)
        return AudioProcessorEditor::mouseWheelMove(event, wheel);

    const auto band = findEqNode(event.position);
    if (band < 0)
        return AudioProcessorEditor::mouseWheelMove(event, wheel);

    selectedEqBand = band;
    auto* id = kEqQIDs[band] != nullptr ? kEqQIDs[band] : kEqGainIDs[band];
    if (auto* parameter = audioProcessor.getAPVTS().getParameter(id))
    {
        const auto range = parameter->getNormalisableRange();
        const auto step = range.interval > 0.0f ? range.interval : (range.end - range.start) / 100.0f;
        const auto direction = wheel.deltaY > 0.0f ? 1.0f : -1.0f;
        const auto current = parameter->convertFrom0to1(parameter->getValue());
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(
            juce::jlimit(range.start, range.end, current + direction * step)));
        parameter->endChangeGesture();
        syncEQKnobsToSelectedBand();
        repaint();
    }
}

// ---------------------------------------------------------------------------
// Parameter listener
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (! applyingSessionChange)
        presetSession.onParameterChanged();

    if (parameterID == VoxlineParameterIDs::polish)
    {
        pendingPolishValue.store(newValue);
        triggerAsyncUpdate();
    }
    else if (parameterID == VoxlineParameterIDs::spaceAmount)
    {
        auto* param = audioProcessor.getAPVTS().getParameter(parameterID);
        if (param)
            spaceAmountLabel.setText(param->getCurrentValueAsText(), juce::dontSendNotification);
    }
    else if (parameterID == VoxlineParameterIDs::spaceMode)
    {
        const int t = juce::jlimit(0, 4, juce::roundToInt(newValue));
        spaceTypeCombo.setSelectedId(t + 1, juce::dontSendNotification);
        triggerAsyncUpdate();
    }
    else
    {
        repaintEQCurve();
    }

}

void VoxlineAudioProcessorEditor::handleAsyncUpdate()
{
    syncSpaceModeControls();
    repaint();
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::applyTheme(const VoxlineTheme& theme)
{
    constexpr int index = 1;
    constexpr bool dark = true;

    // Knobs
    inputGainSlider.setTheme(theme);
    polishSlider.setTheme(theme);
    bodySlider.setTheme(theme);
    claritySlider.setTheme(theme);
    airSlider.setTheme(theme);
    smoothSlider.setTheme(theme);
    compSlider.setTheme(theme);
    driveSlider.setTheme(theme);
    thresholdKnob.setTheme(theme);
    ratioKnob.setTheme(theme);
    attackKnob.setTheme(theme);
    releaseKnob.setTheme(theme);
    preDelayKnob.setTheme(theme);
    spaceHpfKnob.setTheme(theme);
    spaceLpfKnob.setTheme(theme);
    eqFreqKnob.setTheme(theme);
    eqGainKnob.setTheme(theme);
    eqQKnob.setTheme(theme);
    compMixKnob.setTheme(theme);
    deEssFreqKnob.setTheme(theme);
    deEssThresholdKnob.setTheme(theme);
    deEssRangeKnob.setTheme(theme);
    driveToneKnob.setTheme(theme);
    driveMixKnob.setTheme(theme);
    spaceTimeKnob.setTheme(theme);
    spacePreDelayKnob.setTheme(theme);
    spaceWidthKnob.setTheme(theme);
    spaceToneKnob.setTheme(theme);
    spaceDecayKnob.setTheme(theme);
    spaceDuckingKnob.setTheme(theme);
    outputGainSlider.setTheme(theme);
    spaceSlider.setTheme(theme);
    lowCutKnob.setTheme(theme);
    cleanKnob.setTheme(theme);
    deEssKnob.setTheme(theme);

    // Labels — logo + subtitle
    logoLabel.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    subtitleLabel.setFont(juce::FontOptions(13.0f));

    // Panel titles — unified: 14px bold, subtle tracking, Text Primary
    const auto titleFont = juce::Font(juce::FontOptions(14.0f, juce::Font::bold))
                               .withExtraKerningFactor(0.06f);
    inputTitleLabel.setFont(titleFont);
    toneTitleLabel.setFont(titleFont);
    polishTitleLabel.setFont(titleFont);
    outputTitleLabel.setFont(titleFont);
    meterNamesLabel.setFont(titleFont);
    spaceTitleLabel.setFont(titleFont);
    monitorLabel.setFont(titleFont);

    const auto setTextColour = [&](juce::Label& l) { l.setColour(juce::Label::textColourId, theme.textPrimary); };
    setTextColour(logoLabel);
    setTextColour(subtitleLabel);
    setTextColour(inputTitleLabel);
    setTextColour(toneTitleLabel);
    setTextColour(polishTitleLabel);
    setTextColour(outputTitleLabel);
    setTextColour(meterNamesLabel);
    setTextColour(spaceTitleLabel);
    setTextColour(spaceAmountLabel);
    setTextColour(preDelayLabel);
    setTextColour(spaceHpfLabel);
    setTextColour(spaceLpfLabel);
    setTextColour(monitorLabel);

    // Placeholder label fonts
    auto stylePH = [&](juce::Label& l) { l.setFont(juce::FontOptions(10.0f)); };
    stylePH(preDelayLabel); stylePH(spaceHpfLabel); stylePH(spaceLpfLabel);

    // Footer
    footerLabel.setFont(juce::FontOptions(10.0f));
    footerLabel.setColour(juce::Label::textColourId, theme.textMuted.withAlpha(0.5f));

    // === Header — theme-aware image buttons ===
    bypassButton.setThemeIndex(index);
    listenButton.setThemeIndex(index);

    // === Bottom bar utility buttons ===
    const auto inactiveBg = dark ? juce::Colour(0xff171818) : juce::Colour(0xfffaf7f2);
    abButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    abButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);
    for (auto* button : { &presetPreviousButton, &presetNextButton, &presetManageButton,
                          &savePresetButton, &eqResetButton, &eqRangeButton })
    {
        button->setColour(juce::TextButton::buttonColourId, inactiveBg);
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff241714));
        button->setColour(juce::TextButton::textColourOffId, theme.textSecondary);
        button->setColour(juce::TextButton::textColourOnId, theme.accentRose);
    }

    // Monitor buttons — same style but quieter
    auto styleMonitorBtn = [&](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonColourId, inactiveBg);
        b.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
    };
    styleMonitorBtn(monitorAbBtn);
    styleMonitorBtn(monitorListenBtn);
    styleMonitorBtn(monitorBypassBtn);
    advancedButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    advancedButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);

    // === SPACE ===
    VoxlineSpaceSliderLNF::spaceSliderTheme = index;
    spaceSlider.repaint();

    // === Preset dropdown ===
    VoxlinePresetDropdownLNF::currentDropdownTheme = index;

    // === Vocal EQ band buttons (single-theme PNGs, no theme update needed) ===
    presetDropdown.getProperties().set("themeIndex", index);
    presetDropdown.repaint();
    deEssModeCombo.getProperties().set("themeIndex", index);
    driveCharacterCombo.getProperties().set("themeIndex", index);
    spaceTypeCombo.getProperties().set("themeIndex", index);

    // === Toggles ===
    autoGainButton.setThemeIndex(index);
    cleanModeButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);
    cleanModeButton.setColour(juce::ToggleButton::tickColourId, theme.accentLavender);
    // eqOn is hidden until dedicated EQ artwork exists.
    eqOnButton.setThemeIndex(index);

    // === Meters ===
    const auto meterWell = dark ? juce::Colour(0xff111212) : juce::Colour(0xffD5CFC8);
    outputMeter.setColour(VoxlineLevelMeter::backgroundColour, meterWell);
    outputMeter.setColour(VoxlineLevelMeter::foregroundColour, theme.meterMid);
    gainReductionMeter.setColour(VoxlineLevelMeter::backgroundColour, meterWell);
    gainReductionMeter.setColour(VoxlineLevelMeter::foregroundColour, theme.meterLow);

    // === Icons ===
    loadIconDrawables();
    updateAdvancedVisibility();

    repaint();
}

// ---------------------------------------------------------------------------
// Icons
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::loadIconDrawables()
{
    const auto parse = [](const char* data, int size) -> std::unique_ptr<juce::Drawable>
    {
        if (!data || size <= 0) return nullptr;
        auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, size));
        return xml ? juce::Drawable::createFromSVG(*xml) : nullptr;
    };

    cachedBypassIcon = parse(BinaryData::bypass_dark_svg, BinaryData::bypass_dark_svgSize);
    cachedListenIcon = parse(BinaryData::listen_dark_svg, BinaryData::listen_dark_svgSize);
    cachedSettingsIcon.reset();
}

void VoxlineAudioProcessorEditor::paintIcons(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

// ---------------------------------------------------------------------------
// LED dots
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::paintLedDots(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& t = VoxlineTheme::get(currentThemeIndex);
    const int numDots = 7;
    const float dotR = 3.5f;
    const float spacing = (float)bounds.getWidth() / (float)(numDots - 1);
    const float cy = static_cast<float>(bounds.getCentreY());

    const int activeDots = juce::jlimit(0, numDots, juce::roundToInt(inputLedLevel * (float)numDots));

    for (int i = 0; i < numDots; ++i)
    {
        const float cx = static_cast<float>(bounds.getX()) + static_cast<float>(i) * spacing;
        if (i < activeDots)
        {
            // Gradient from green-ish (low) to accentLavender (high)
            const auto gradientAmount = static_cast<float>(i) / static_cast<float>(numDots - 1);
            g.setColour(juce::Colour::fromFloatRGBA(0.55f + gradientAmount * 0.13f,
                                                     0.72f + gradientAmount * 0.11f,
                                                     0.55f + gradientAmount * 0.45f, 1.0f));
            g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        }
        else
        {
            g.setColour(t.textMuted.withAlpha(0.35f));
            g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.0f);
        }
    }
}

// ---------------------------------------------------------------------------
// Meter timer
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::timerCallback()
{
    auto& proc = audioProcessor;
    const auto inPeak = proc.inputPeak.load();
    const auto outPeak = proc.outputPeak.load();
    const auto gr = proc.gainReduction.load();

    inputLedLevel = inPeak;

    outputMeter.setLevel(outPeak);
    gainReductionMeter.setLevel(gr);
    updateSpectrum();

    const auto monitorMode = proc.getMonitorState().mode();
    eqBandSoloButton.setToggleState(
        monitorMode == VoxlineState::MonitorMode::eqBandSolo
            && proc.getMonitorState().eqBand() == selectedEqBand,
        juce::dontSendNotification);
    deEssListenButton.setToggleState(monitorMode == VoxlineState::MonitorMode::deEssListenS,
                                     juce::dontSendNotification);

    repaint();
}

void VoxlineAudioProcessorEditor::updateSpectrum()
{
    audioProcessor.copyAnalyzerSamples(spectrumInput);
    std::fill(spectrumFftData.begin(), spectrumFftData.end(), 0.0f);
    std::copy(spectrumInput.begin(), spectrumInput.end(), spectrumFftData.begin());
    spectrumWindow.multiplyWithWindowingTable(spectrumFftData.data(),
                                               VoxlineAudioProcessor::analyzerFftSize);
    spectrumFft.performFrequencyOnlyForwardTransform(spectrumFftData.data());

    const auto sampleRate = static_cast<float>(juce::jmax(1.0, audioProcessor.getSampleRate()));
    for (size_t i = 0; i < spectrumDisplay.size(); ++i)
    {
        const auto norm = static_cast<float>(i) / static_cast<float>(spectrumDisplay.size() - 1);
        const auto frequency = 20.0f * std::pow(1000.0f, norm);
        const auto bin = juce::jlimit(1, VoxlineAudioProcessor::analyzerFftSize / 2 - 1,
            juce::roundToInt(frequency * VoxlineAudioProcessor::analyzerFftSize / sampleRate));
        const auto magnitude = spectrumFftData[static_cast<size_t>(bin)]
                             / static_cast<float>(VoxlineAudioProcessor::analyzerFftSize);
        const auto db = juce::Decibels::gainToDecibels(magnitude, -72.0f);
        spectrumDisplay[i] += (db - spectrumDisplay[i]) * (db > spectrumDisplay[i] ? 0.45f : 0.12f);
    }
}

// ---------------------------------------------------------------------------
// Preset system
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::setAdvancedSection(AdvancedSection section)
{
    if (advancedSection != section)
    {
        audioProcessor.getMonitorState().clear();
        eqBandSoloButton.setToggleState(false, juce::dontSendNotification);
        deEssListenButton.setToggleState(false, juce::dontSendNotification);
    }
    advancedSection = section;
    updateAdvancedVisibility();
    repaint();
}

void VoxlineAudioProcessorEditor::setAdvancedOpen(bool shouldOpen)
{
    advancedOpen = shouldOpen;
    setSize(VoxlineLayout::editorWidth, advancedOpen ? 940 : VoxlineLayout::editorHeight);
    updateAdvancedVisibility();
    repaint();
}

void VoxlineAudioProcessorEditor::updateAdvancedVisibility()
{
    advancedButton.setButtonText(advancedOpen ? "ADVANCED  ^" : "ADVANCED  v");

    for (auto* button : { &advancedEqButton, &advancedCompButton,
                          &advancedDeEssButton, &advancedDriveButton, &advancedSpaceButton })
        button->setVisible(advancedOpen);

    const auto eq = advancedOpen && advancedSection == AdvancedSection::eq;
    juce::Component* eqComponents[] = { &eqHpfButton, &eqLowButton, &eqMudButton,
                                        &eqPresButton, &eqAirButton, &eqLpfButton,
                                        &eqFreqKnob, &eqGainKnob, &eqQKnob, &eqOnButton,
                                        &eqResetButton, &eqRangeButton, &eqBandEnableButton,
                                        &eqBandSoloButton };
    for (auto* component : eqComponents)
        component->setVisible(eq);

    const auto comp = advancedOpen && advancedSection == AdvancedSection::comp;
    juce::Component* compComponents[] = { &thresholdKnob, &ratioKnob,
                                          &attackKnob, &releaseKnob, &compMixKnob,
                                          &compMakeupKnob, &compAutoMakeupButton };
    for (auto* component : compComponents)
        component->setVisible(comp);

    const auto deEss = advancedOpen && advancedSection == AdvancedSection::deEss;
    deEssFreqKnob.setVisible(deEss);
    deEssThresholdKnob.setVisible(deEss);
    deEssRangeKnob.setVisible(deEss);
    deEssModeCombo.setVisible(deEss);
    deEssListenButton.setVisible(deEss);

    const auto drive = advancedOpen && advancedSection == AdvancedSection::drive;
    driveToneKnob.setVisible(drive);
    driveMixKnob.setVisible(drive);
    driveOutputTrimKnob.setVisible(drive);
    driveCharacterCombo.setVisible(drive);
    driveLevelMatchButton.setVisible(drive);

    const auto space = advancedOpen && advancedSection == AdvancedSection::space;
    const auto* modeParameter = audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::spaceMode);
    const auto spaceMode = juce::roundToInt(modeParameter != nullptr ? modeParameter->load() : 0.0f);
    spaceTimeKnob.setVisible(space);
    spacePreDelayKnob.setVisible(space && spaceMode != 3);
    spaceWidthKnob.setVisible(space && spaceMode != 4);
    spaceToneKnob.setVisible(space);
    spaceDecayKnob.setVisible(space && spaceMode != 4);
    spaceDuckingKnob.setVisible(space && spaceMode != 4);
    spaceTypeCombo.setVisible(space);
    spaceMonoSafetyButton.setVisible(space && spaceMode == 4);

    // Old V2 placeholders are intentionally removed from the fast interface.
    juce::Component* retiredComponents[] = { &lowCutKnob, &cleanKnob, &deEssKnob,
                                             &preDelayKnob, &spaceHpfKnob, &spaceLpfKnob,
                                             &monitorAbBtn, &monitorListenBtn, &monitorBypassBtn,
                                             &cleanModeButton };
    for (auto* component : retiredComponents)
        component->setVisible(false);

    const auto& theme = VoxlineTheme::get(currentThemeIndex);
    const auto inactive = currentThemeIndex != 0 ? juce::Colour(0xff171818) : juce::Colour(0xffEEE7DF);
    const auto active = currentThemeIndex != 0 ? juce::Colour(0xff171818) : juce::Colour(0xffF3E7DF);
    struct Nav { juce::TextButton* button; AdvancedSection section; };
    const Nav nav[] = { {&advancedEqButton, AdvancedSection::eq},
                        {&advancedCompButton, AdvancedSection::comp},
                        {&advancedDeEssButton, AdvancedSection::deEss},
                        {&advancedDriveButton, AdvancedSection::drive},
                        {&advancedSpaceButton, AdvancedSection::space} };
    for (const auto& item : nav)
    {
        item.button->setColour(juce::TextButton::buttonColourId,
                               item.section == advancedSection ? active : inactive);
        item.button->setColour(juce::TextButton::textColourOffId,
                               item.section == advancedSection ? juce::Colour(0xffF06A3D) : theme.textSecondary);
    }
}

void VoxlineAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &presetPreviousButton) { selectRelativePreset(-1); return; }
    if (button == &presetNextButton) { selectRelativePreset(1); return; }
    if (button == &presetManageButton)
    {
        const auto current = presetSession.presentation().currentName;
        if (current == "Untitled")
            return;
        juce::PopupMenu menu;
        menu.addItem(1, "Rename…");
        menu.addItem(2, "Delete…");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetManageButton),
            [safe = juce::Component::SafePointer<VoxlineAudioProcessorEditor>(this), current](int result)
            {
                if (safe == nullptr)
                    return;
                if (result == 1)
                    safe->showPresetNameDialog(true);
                else if (result == 2)
                {
                    const auto options = juce::MessageBoxOptions::makeOptionsYesNo(
                        juce::MessageBoxIconType::WarningIcon, "Delete User Preset",
                        "Delete \"" + current + "\"?", "Delete", "Cancel", nullptr);
                    juce::AlertWindow::showAsync(options,
                        [safe, current](int choice)
                        {
                            if (safe != nullptr && choice == 1)
                            {
                                safe->showResult(safe->presetSession.remove(current));
                                safe->refreshPresetMenu();
                                safe->repaint();
                            }
                        });
                }
            });
        return;
    }
    if (button == &savePresetButton) { saveUserPreset(); return; }
    if (button == &advancedButton)
    {
        setAdvancedOpen(! advancedOpen);
        return;
    }
    if (button == &advancedEqButton) { setAdvancedSection(AdvancedSection::eq); return; }
    if (button == &advancedCompButton) { setAdvancedSection(AdvancedSection::comp); return; }
    if (button == &advancedDeEssButton) { setAdvancedSection(AdvancedSection::deEss); return; }
    if (button == &advancedDriveButton) { setAdvancedSection(AdvancedSection::drive); return; }
    if (button == &advancedSpaceButton) { setAdvancedSection(AdvancedSection::space); return; }
    if (button == &abButton) { toggleAb(); return; }
    if (button == &eqRangeButton)
    {
        auto& apvts = audioProcessor.getAPVTS();
        for (int band = 0; band < 6; ++band)
            for (auto* id : { kEqFreqIDs[band], kEqGainIDs[band], kEqQIDs[band], kEqEnabledIDs[band] })
                if (id != nullptr)
                    if (auto* parameter = apvts.getParameter(id))
                        parameter->setValueNotifyingHost(parameter->getDefaultValue());
        if (auto* parameter = apvts.getParameter(VoxlineParameterIDs::eqEnabled))
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
        syncEQKnobsToSelectedBand();
        repaint();
        return;
    }
    if (button == &eqBandSoloButton)
    {
        auto& monitor = audioProcessor.getMonitorState();
        monitor.setEqBandSolo(eqBandSoloButton.getToggleState() ? selectedEqBand : -1);
        return;
    }
    if (button == &deEssListenButton)
    {
        audioProcessor.getMonitorState().setDeEssListen(deEssListenButton.getToggleState());
        return;
    }
    if (button == &outputClipClearButton)
    {
        audioProcessor.clearOutputClipHold();
        return;
    }
    if (button == &eqResetButton)
    {
        auto& apvts = audioProcessor.getAPVTS();
        for (auto* id : { kEqFreqIDs[selectedEqBand], kEqGainIDs[selectedEqBand],
                          kEqQIDs[selectedEqBand], kEqEnabledIDs[selectedEqBand] })
            if (id != nullptr)
                if (auto* parameter = apvts.getParameter(id))
                    parameter->setValueNotifyingHost(parameter->getDefaultValue());
        syncEQKnobsToSelectedBand();
        repaint();
        return;
    }

    // EQ band button selection — radio behavior
    VoxlineImageButton* bandBtns[] = { &eqHpfButton, &eqLowButton, &eqMudButton,
                                       &eqPresButton, &eqAirButton, &eqLpfButton };
    for (int i = 0; i < 6; ++i)
    {
        if (button == bandBtns[i])
        {
            selectedEqBand = i;
            for (int j = 0; j < 6; ++j)
                bandBtns[j]->setToggleState(j == i, juce::dontSendNotification);
            eqBandEnabledAttachment.reset();
            eqBandEnabledAttachment = std::make_unique<ButtonAttachment>(
                audioProcessor.getAPVTS(), kEqEnabledIDs[selectedEqBand], eqBandEnableButton);
            eqBandSoloButton.setToggleState(
                audioProcessor.getMonitorState().mode() == VoxlineState::MonitorMode::eqBandSolo
                && audioProcessor.getMonitorState().eqBand() == selectedEqBand,
                juce::dontSendNotification);
            syncEQKnobsToSelectedBand();
            repaint();
            return;
        }
    }
}

void VoxlineAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetDropdown)
    {
        if (presetDropdown.getSelectedId() < 100)
            return;
        requestPresetSelection(presetDropdown.getText());
    }
}

void VoxlineAudioProcessorEditor::selectRelativePreset(int delta)
{
    const auto presentation = presetSession.presentation();
    if (presentation.names.isEmpty())
    {
        showResult(juce::Result::fail("No user presets"));
        return;
    }

    auto currentIndex = presentation.names.indexOf(presentation.currentName);
    if (currentIndex < 0)
        currentIndex = delta < 0 ? 0 : presentation.names.size() - 1;
    const auto target = (currentIndex + delta + presentation.names.size()) % presentation.names.size();
    requestPresetSelection(presentation.names[target]);
}

void VoxlineAudioProcessorEditor::selectPresetNow(const juce::String& presetName,
                                                   VoxlineState::UnsavedAction action)
{
    applyingSessionChange = true;
    const auto result = presetSession.select(presetName, action);
    applyingSessionChange = false;
    showResult(result);
    syncEQKnobsToSelectedBand();
    refreshPresetMenu();
    repaint();
}

void VoxlineAudioProcessorEditor::requestPresetSelection(const juce::String& presetName)
{
    if (! presetSession.isEdited())
    {
        selectPresetNow(presetName, VoxlineState::UnsavedAction::discard);
        return;
    }

    if (unsavedPresetDialog != nullptr)
        return;

    unsavedPresetDialog = std::make_unique<juce::AlertWindow>(
        "Unsaved Preset", "Keep the edits before switching presets?",
        juce::MessageBoxIconType::WarningIcon, this);
    unsavedPresetDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    unsavedPresetDialog->addButton("Discard", 2);
    unsavedPresetDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    unsavedPresetDialog->enterModalState(true,
        juce::ModalCallbackFunction::create(
            [safe = juce::Component::SafePointer<VoxlineAudioProcessorEditor>(this), presetName](int choice)
            {
                if (safe == nullptr || safe->unsavedPresetDialog == nullptr)
                    return;
                safe->unsavedPresetDialog->setVisible(false);
                safe->unsavedPresetDialog.reset();
                if (choice == 2)
                    safe->selectPresetNow(presetName, VoxlineState::UnsavedAction::discard);
                else if (choice == 1)
                {
                    if (safe->presetSession.presentation().currentName != "Untitled")
                        safe->selectPresetNow(presetName, VoxlineState::UnsavedAction::save);
                    else
                        safe->showPresetNameDialog(false,
                            [safe, presetName](bool saved)
                            {
                                if (saved && safe != nullptr)
                                    safe->selectPresetNow(presetName, VoxlineState::UnsavedAction::discard);
                            });
                }
                else
                {
                    safe->refreshPresetMenu();
                    safe->repaint();
                }
            }), false);
}

void VoxlineAudioProcessorEditor::saveUserPreset()
{
    showPresetNameDialog(false);
}

void VoxlineAudioProcessorEditor::showPresetNameDialog(bool rename, std::function<void(bool)> completion)
{
    if (presetNameDialog != nullptr)
        return;

    const auto currentName = presetSession.presentation().currentName;
    if (rename && currentName == "Untitled")
        return;
    presetNameDialogRenamesCurrent = rename;
    presetNameDialogCompletion = std::move(completion);
    presetNameDialog = std::make_unique<juce::AlertWindow>(
        rename ? "Rename User Preset" : "Save User Preset", "Give this sound a name.",
        juce::MessageBoxIconType::NoIcon, this);
    presetNameDialog->addTextEditor("name", currentName == "Untitled" ? "" : currentName, "Name:");
    presetNameDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    presetNameDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    presetNameDialog->enterModalState(true,
        juce::ModalCallbackFunction::create(
            [safe = juce::Component::SafePointer<VoxlineAudioProcessorEditor>(this)](int result)
            {
                if (safe == nullptr || safe->presetNameDialog == nullptr)
                    return;
                const auto name = safe->presetNameDialog->getTextEditorContents("name").trim();
                const auto renameCurrent = safe->presetNameDialogRenamesCurrent;
                auto afterNameDialog = std::move(safe->presetNameDialogCompletion);
                safe->presetNameDialog->setVisible(false);
                safe->presetNameDialog.reset();
                if (result == 1)
                {
                    const auto saveResult = renameCurrent ? safe->presetSession.renameCurrent(name)
                                                          : safe->presetSession.saveAs(name);
                    safe->showResult(saveResult);
                    safe->refreshPresetMenu();
                    safe->repaint();
                    if (afterNameDialog)
                        afterNameDialog(saveResult.wasOk());
                }
                else if (afterNameDialog)
                    afterNameDialog(false);
            }), false);
}

void VoxlineAudioProcessorEditor::toggleAb()
{
    auto& ab = audioProcessor.getAbState();
    applyingSessionChange = true;
    ab.captureActiveSlot();
    ab.select(ab.activeSlot() == VoxlineState::AbSlot::a
        ? VoxlineState::AbSlot::b : VoxlineState::AbSlot::a);
    applyingSessionChange = false;
    presetSession.onAbChanged();
    abButton.setButtonText(ab.activeSlot() == VoxlineState::AbSlot::a ? "A" : "B");
    syncEQKnobsToSelectedBand();
    repaint();
}

void VoxlineAudioProcessorEditor::refreshPresetMenu()
{
    const auto presentation = presetSession.presentation();
    presetDropdown.clear(juce::dontSendNotification);
    if (presentation.names.isEmpty())
    {
        presetDropdown.addItem("No User Presets", 1);
        presetDropdown.setItemEnabled(1, false);
        presetDropdown.setText("Untitled", juce::dontSendNotification);
    }
    else
    {
        for (int index = 0; index < presentation.names.size(); ++index)
            presetDropdown.addItem(presentation.names[index], 100 + index);
        presetDropdown.setText(presentation.currentName, juce::dontSendNotification);
    }
}

void VoxlineAudioProcessorEditor::showResult(const juce::Result& result)
{
    if (result.failed())
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "VOXLINE", result.getErrorMessage(), "OK", this);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::configureKnob(VoxlineCustomKnob& knob)           { addAndMakeVisible(knob); }
void VoxlineAudioProcessorEditor::configureButton(juce::ToggleButton& b, const juce::String& t) { b.setButtonText(t); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configureHeaderButton(juce::TextButton& b, const juce::String& t) { b.setButtonText(t); b.setLookAndFeel(&getButtonLookAndFeel()); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configurePresetButton(juce::TextButton& b, const juce::String& t, bool) { b.setButtonText(t); b.setLookAndFeel(&getButtonLookAndFeel()); b.setEnabled(true); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configureTextLabel(juce::Label& l, const juce::String& t, juce::Justification j) { l.setText(t, juce::dontSendNotification); l.setJustificationType(j); addAndMakeVisible(l); }

void VoxlineAudioProcessorEditor::repaintEQCurve()
{
    const auto area = VoxlineLayout::eqPanel.expanded(2);
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        repaint(area);
        return;
    }

    juce::Component::SafePointer<VoxlineAudioProcessorEditor> safeThis(this);
    juce::MessageManager::callAsync([safeThis, area]
    {
        if (safeThis != nullptr)
            safeThis->repaint(area);
    });
}

void VoxlineAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    auto& apvts = audioProcessor.getAPVTS();
    if (slider == &eqFreqKnob || slider == &eqGainKnob || slider == &eqQKnob)
    {
        const int sel = selectedEqBand;
        const float val = (float)slider->getValue();

        if (slider == &eqFreqKnob)
        {
            if (auto* p = apvts.getParameter(kEqFreqIDs[sel]))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        }
        else if (slider == &eqGainKnob)
        {
            if (auto* p = apvts.getParameter(kEqGainIDs[sel]))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        }
        else if (kEqQIDs[sel] != nullptr)
        {
            if (auto* p = apvts.getParameter(kEqQIDs[sel]))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        }
        repaintEQCurve();
    }
}

void VoxlineAudioProcessorEditor::syncSpaceModeControls()
{
    const auto* mode = audioProcessor.getAPVTS().getRawParameterValue(VoxlineParameterIDs::spaceMode);
    const auto value = juce::roundToInt(mode != nullptr ? mode->load() : 0.0f);
    const auto sizeOrTime = value == 3 ? VoxlineParameterIDs::spaceSlapTime
                         : value == 4 ? VoxlineParameterIDs::spacePreDelay
                                      : VoxlineParameterIDs::spaceSize;
    const auto preDelayOrSpread = value == 4 ? VoxlineParameterIDs::spaceWidth
                                             : VoxlineParameterIDs::spacePreDelay;
    const auto decayOrFeedback = value == 3 ? VoxlineParameterIDs::spaceFeedback
                                            : VoxlineParameterIDs::spaceDecay;
    spaceTimeAttachment.reset();
    spacePreDelayAttachment.reset();
    spaceDecayAttachment.reset();
    spaceTimeAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), sizeOrTime, spaceTimeKnob);
    spacePreDelayAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), preDelayOrSpread, spacePreDelayKnob);
    spaceDecayAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), decayOrFeedback, spaceDecayKnob);
    updateAdvancedVisibility();
}

void VoxlineAudioProcessorEditor::syncEQKnobsToSelectedBand()
{
    auto& apvts = audioProcessor.getAPVTS();
    const int sel = selectedEqBand;

    if (auto* p = apvts.getParameter(kEqFreqIDs[sel]))
    {
        const auto range = p->getNormalisableRange();
        eqFreqKnob.setRange(range.start, range.end, range.interval);
        eqFreqKnob.setSkewFactor(range.skew);
        eqFreqKnob.textFromValueFunction = [](double value)
        {
            return value >= 1000.0 ? juce::String(value / 1000.0, value < 10000.0 ? 2 : 1) + " kHz"
                                   : juce::String(value, 0) + " Hz";
        };
        eqFreqKnob.valueFromTextFunction = [](const juce::String& text)
        {
            auto cleaned = text.trim().toLowerCase();
            const auto multiplier = cleaned.contains("k") ? 1000.0 : 1.0;
            return cleaned.retainCharacters("0123456789.-").getDoubleValue() * multiplier;
        };
        eqFreqKnob.setValue(p->convertFrom0to1(p->getValue()), juce::dontSendNotification);
    }
    if (auto* p = apvts.getParameter(kEqGainIDs[sel]))
    {
        if (sel == 0 || sel == 5)
        {
            eqGainKnob.textFromValueFunction = [](double index)
            {
                return juce::String(12 * (juce::roundToInt(index) + 1)) + " dB/oct";
            };
            eqGainKnob.valueFromTextFunction = [](const juce::String& text)
            {
                return juce::jmax(0.0, text.retainCharacters("0123456789").getDoubleValue() / 12.0 - 1.0);
            };
        }
        else
        {
            eqGainKnob.textFromValueFunction = [](double value)
            {
                return (value > 0.0 ? "+" : "") + juce::String(value, 1) + " dB";
            };
            eqGainKnob.valueFromTextFunction = [](const juce::String& text)
            {
                return text.retainCharacters("0123456789.-").getDoubleValue();
            };
        }
        eqGainKnob.setRange(p->getNormalisableRange().start, p->getNormalisableRange().end, p->getNormalisableRange().interval);
        eqGainKnob.setValue(p->convertFrom0to1(p->getValue()), juce::dontSendNotification);
    }
    if (kEqQIDs[sel] != nullptr)
    {
        if (auto* p = apvts.getParameter(kEqQIDs[sel]))
        {
            eqQKnob.setRange(p->getNormalisableRange().start, p->getNormalisableRange().end,
                             p->getNormalisableRange().interval);
            eqQKnob.textFromValueFunction = [](double value) { return juce::String(value, 2); };
            eqQKnob.valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue(); };
            eqQKnob.setValue(p->convertFrom0to1(p->getValue()), juce::dontSendNotification);
            eqQKnob.setEnabled(true);
        }
    }
    else
    {
        eqQKnob.setRange(0.0, 1.0, 0.01);
        eqQKnob.setValue(0.5, juce::dontSendNotification);
        eqQKnob.setEnabled(false);
    }
}

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/Layout.h"
#include <cmath>

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
        g.setColour(on ? juce::Colour(0xffD86F96).withAlpha(0.2f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle(b, 7.0f);
        g.setColour(on ? juce::Colour(0xffD86F96) : juce::Colour(0xffaaaaaa).withAlpha(0.4f));
        g.drawRoundedRectangle(b, 7.0f, 1.0f);

        g.setFont(juce::FontOptions(12.0f));
        g.setColour(button.findColour(on ? juce::ToggleButton::tickColourId : juce::ToggleButton::textColourId));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
};

static VoxlineToggleLookAndFeel voxlineToggleLNF;

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

        const auto fillOff = dark ? juce::Colour(0xff2a2635) : juce::Colour(0xffe8e0d4);
        const auto fillOn  = juce::Colour(0xffD86F96).withAlpha(0.22f);
        const auto borderOff = dark ? juce::Colour(0xff3d3950) : juce::Colour(0xffc8bfb4);
        const auto textOn  = juce::Colour(0xffD86F96);
        const auto textOff = dark ? juce::Colour(0xff9d99a8) : juce::Colour(0xff666666);

        g.setColour(on ? fillOn : fillOff);
        g.fillRoundedRectangle(b, 7.0f);
        g.setColour(on ? textOn : borderOff);
        g.drawRoundedRectangle(b, 7.0f, 1.0f);

        g.setFont(juce::FontOptions(12.0f));
        g.setColour(on ? textOn : textOff);
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    static int currentAutoGainTheme;
};

static VoxlineAutoGainLNF voxlineAutoGainLNF;
int VoxlineAutoGainLNF::currentAutoGainTheme = 0;

// ---------------------------------------------------------------------------
// Compact horizontal slider LookAndFeel for SPACE
// ---------------------------------------------------------------------------
struct VoxlineSpaceSliderLNF final : juce::LookAndFeel_V4
{
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float, float,
                          const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
        const auto trackY = b.getCentreY();
        const auto trackH = 2.5f;
        const auto thumbR = 6.0f;
        const auto& t = VoxlineTheme::get(spaceSliderTheme);
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
static VoxlineSpaceSliderLNF voxlineSpaceSliderLNF;

// ---------------------------------------------------------------------------
// Pill-styled ComboBox LookAndFeel for preset dropdown
// ---------------------------------------------------------------------------
struct VoxlinePresetDropdownLNF final : juce::LookAndFeel_V4
{
    void drawComboBox(juce::Graphics& g, int w, int h, bool isDown, int, int, int, int, juce::ComboBox& box) override
    {
        auto& t = VoxlineTheme::get(box.getProperties().getWithDefault("themeIndex", 0));
        const auto dark = (t.editorBg.getBrightness() < 0.3f);

        g.setColour(dark ? juce::Colour(0xff2f2c38) : juce::Colour(0xffece5de));
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

        g.setColour(isHighlighted ? t.accentRose.withAlpha(dark ? 0.25f : 0.15f) : (dark ? juce::Colour(0xff181622) : juce::Colour(0xffF7F0E7)));
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

static VoxlinePresetDropdownLNF voxlineDropdownLNF;

// ---------------------------------------------------------------------------
// Theme toggle — simple Component, draws sun/moon directly
// ---------------------------------------------------------------------------
struct ThemeToggleComp final : juce::Component
{
    VoxlineAudioProcessorEditor& owner;
    explicit ThemeToggleComp(VoxlineAudioProcessorEditor& o) : owner(o) {}

    void paint(juce::Graphics& g) override
    {
        const auto& t = VoxlineTheme::get(owner.currentThemeIndex);
        const auto dark = (owner.currentThemeIndex != 0);
        const auto b = getLocalBounds().toFloat();

        g.setColour(dark ? juce::Colour(0x00ffffff) : juce::Colour(0xffece5de));
        g.fillRoundedRectangle(b, 8.0f);
        g.setColour(t.panelBorder);
        g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.0f);

        const float cx = b.getCentreX(), cy = b.getCentreY(), r = 8.0f;
        juce::Path icon;
        if (dark)
        {
            icon.addEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            for (int i = 0; i < 8; ++i)
            {
                auto a = juce::MathConstants<float>::twoPi * (float)i / 8.0f;
                const auto sa = std::sin(a), ca = std::cos(a);
                icon.addLineSegment({cx + (r + 1.5f) * ca, cy + (r + 1.5f) * sa,
                                     cx + (r + 5.0f) * ca, cy + (r + 5.0f) * sa}, 2.0f);
            }
        }
        else
        {
            juce::Path outer, inner;
            outer.addEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            inner.addEllipse(cx - r + 4.0f, cy - r - 1.0f, r * 2.0f, r * 2.0f);
            icon = outer;
            icon.addPath(inner);
            icon.setUsingNonZeroWinding(false);
        }
        g.setColour(t.textPrimary);
        g.strokePath(icon, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void mouseUp(const juce::MouseEvent&) override { owner.cycleTheme(); }
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
VoxlineAudioProcessorEditor::VoxlineAudioProcessorEditor(VoxlineAudioProcessor& audioProcessorToEdit)
    : AudioProcessorEditor(&audioProcessorToEdit),
      audioProcessor(audioProcessorToEdit)
{
    auto& apvts = audioProcessor.getAPVTS();

    configureTextLabel(logoLabel, "VOXLINE", juce::Justification::centredLeft);
    configureTextLabel(subtitleLabel, "Complete Vocal Channel", juce::Justification::centredLeft);

    // Theme toggle icon
    themeToggle = std::make_unique<ThemeToggleComp>(*this);
    addAndMakeVisible(*themeToggle);

    // Preset dropdown in bottom bar
    presetDropdown.setLookAndFeel(&voxlineDropdownLNF);
    presetDropdown.setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    presetDropdown.addItemList({"Clean","Basement Take","Dirty Lead","Cold Plug","Rage Cut","Muddy Trap","Cyber Vox","Noir Vocal","Tape Rap"}, 1);
    presetDropdown.setSelectedId(1, juce::dontSendNotification); // Clean
    presetDropdown.getProperties().set("themeIndex", 0);
    presetDropdown.addListener(this);
    addAndMakeVisible(presetDropdown);

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
    eqLowButton.setToggleState(true, juce::dontSendNotification);  // LOW active by default

    // Placeholder controls (UI only, no DSP yet)
    configureTextLabel(thresholdLabel, "", juce::Justification::centred);  // text now drawn in paint()
    configureTextLabel(preDelayLabel, "PRE-DELAY\n15 ms", juce::Justification::centred);
    configureTextLabel(spaceHpfLabel, "HPF\n200 Hz", juce::Justification::centred);
    configureTextLabel(spaceLpfLabel, "LPF\n8.0 kHz", juce::Justification::centred);
    configureTextLabel(monitorLabel, "MONITOR", juce::Justification::centredLeft);

    configurePresetButton(abButton, "A/B");

    abButton.addListener(this);

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
    deEssKnob.setShowInternalLabel(false);
    deEssKnob.setShowInternalValue(false);
    deEssKnob.setRange(0.0, 100.0, 1.0);
    deEssKnob.setValue(25.0, juce::dontSendNotification);

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
    configureKnob(polishSlider);
    configureKnob(bodySlider);
    configureKnob(claritySlider);
    configureKnob(airSlider);
    configureKnob(smoothSlider);

    // Old EQ DSP knobs hidden — controlled by band buttons, values shown in selected-band panel
    bodySlider.setVisible(false);
    claritySlider.setVisible(false);
    airSlider.setVisible(false);
    smoothSlider.setVisible(false);
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

    // SPACE control in bottom bar
    spaceTypeCombo.addItemList({"Tight", "Slap", "Wide"}, 1);
    spaceTypeCombo.setSelectedId(1, juce::dontSendNotification);
    spaceTypeCombo.addListener(this);
    spaceTypeCombo.setLookAndFeel(&voxlineDropdownLNF);
    spaceTypeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    spaceTypeCombo.getProperties().set("themeIndex", 0);
    addAndMakeVisible(spaceTypeCombo);

    // SPACE horizontal slider
    spaceSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    spaceSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    spaceSlider.setRange(0.0, 100.0, 1.0);
    spaceSlider.setLookAndFeel(&voxlineSpaceSliderLNF);
    addAndMakeVisible(spaceSlider);

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

    // Monitor buttons
    configurePresetButton(monitorAbBtn, "A/B");
    configurePresetButton(monitorListenBtn, "Listen");
    configurePresetButton(monitorBypassBtn, "Bypass");

    configureTextLabel(spaceAmountLabel, "24%", juce::Justification::centredRight);

    // Footer
    configureTextLabel(footerLabel, "VOXLINE 2.0.0  |  SADTONY", juce::Justification::centred);



    // Bypass — theme-aware image button
    addAndMakeVisible(bypassButton);
    bypassButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::bypass_normal_dark_png,  BinaryData::bypass_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::bypass_normal_light_png, BinaryData::bypass_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::bypass_active_dark_png,  BinaryData::bypass_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::bypass_active_light_png, BinaryData::bypass_active_light_pngSize)
    );
    bypassButton.setThemeIndex(0);
    bypassButton.addListener(this);

    // Listen — theme-aware image button
    addAndMakeVisible(listenButton);
    listenButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::listen_normal_dark_png,  BinaryData::listen_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::listen_normal_light_png, BinaryData::listen_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::listen_active_dark_png,  BinaryData::listen_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::listen_active_light_png, BinaryData::listen_active_light_pngSize)
    );
    listenButton.setThemeIndex(0);
    listenButton.addListener(this);

    // EQ On/Off — theme-aware image button
    addAndMakeVisible(eqOnButton);
    eqOnButton.setThemeImages(
        juce::ImageCache::getFromMemory(BinaryData::on_normal_dark_png,  BinaryData::on_normal_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_normal_light_png, BinaryData::on_normal_light_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_active_dark_png,  BinaryData::on_active_dark_pngSize),
        juce::ImageCache::getFromMemory(BinaryData::on_active_light_png, BinaryData::on_active_light_pngSize)
    );
    eqOnButton.setThemeIndex(0);
    eqOnButton.setToggleState(true, juce::dontSendNotification);
    eqOnButton.addListener(this);

    // Auto-gain / Clean — remain ToggleButtons (no PNG assets yet)
    configureButton(autoGainButton, "ON");
    autoGainButton.setLookAndFeel(&voxlineAutoGainLNF);
    configureButton(cleanModeButton, "Clean");
    cleanModeButton.setLookAndFeel(&voxlineToggleLNF);

    // EQ band knobs
    configureKnob(eqFreqKnob);
    configureKnob(eqGainKnob);
    eqFreqKnob.setShowInternalLabel(false);
    eqFreqKnob.setShowInternalValue(false);
    eqGainKnob.setShowInternalLabel(false);
    eqGainKnob.setShowInternalValue(false);
    eqFreqKnob.addListener(this);
    eqGainKnob.addListener(this);
    syncEQKnobsToSelectedBand();

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

    autoGainAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::autoGain, autoGainButton);
    bypassAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::bypass, bypassButton);
    cleanModeAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::cleanMode, cleanModeButton);
    listenAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::listen, listenButton);
    eqEnabledAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::eqEnabled, eqOnButton);

    apvts.addParameterListener(VoxlineParameterIDs::polish, this);
    apvts.addParameterListener(VoxlineParameterIDs::inputGain, this);
    apvts.addParameterListener(VoxlineParameterIDs::body, this);
    apvts.addParameterListener(VoxlineParameterIDs::clarity, this);
    apvts.addParameterListener(VoxlineParameterIDs::air, this);
    apvts.addParameterListener(VoxlineParameterIDs::smooth, this);
    apvts.addParameterListener(VoxlineParameterIDs::comp, this);
    apvts.addParameterListener(VoxlineParameterIDs::drive, this);
    apvts.addParameterListener(VoxlineParameterIDs::outputGain, this);
    apvts.addParameterListener(VoxlineParameterIDs::autoGain, this);
    apvts.addParameterListener(VoxlineParameterIDs::bypass, this);
    apvts.addParameterListener(VoxlineParameterIDs::listen, this);
    apvts.addParameterListener(VoxlineParameterIDs::spaceAmount, this);
    apvts.addParameterListener(VoxlineParameterIDs::spaceType, this);

    // Init A/B snapshots from current APVTS values
    captureSnapshot(snapshotA);
    snapshotB = snapshotA;

    setSize(VoxlineLayout::editorWidth, VoxlineLayout::editorHeight);
    setResizable(false, false);

    addKeyListener(this);
    setWantsKeyboardFocus(true);

    loadIconDrawables(false);
    applyTheme(VoxlineTheme::light, 0);
    startTimerHz(30); // meter refresh
}

VoxlineAudioProcessorEditor::~VoxlineAudioProcessorEditor()
{
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::polish, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::inputGain, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::body, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::clarity, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::air, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::smooth, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::comp, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::drive, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::outputGain, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::autoGain, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::bypass, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::listen, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::spaceAmount, this);
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::spaceType, this);
    removeKeyListener(this);
    autoGainButton.setLookAndFeel(nullptr);
    presetDropdown.removeListener(this);
    presetDropdown.setLookAndFeel(nullptr);
    stopTimer();
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto& t = VoxlineTheme::get(currentThemeIndex);
    g.fillAll(t.editorBg);

    // Top bar — just a simple background
    g.setColour(t.mainCardBg);
    g.fillRoundedRectangle(20, 20, 1360, 78, 24.0f);

    // Six panels
    g.setColour(t.panelBg);
    g.fillRoundedRectangle(VoxlineLayout::inputPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::polishPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::outputPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::eqPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::dynamicsPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::spacePanel.toFloat(), VoxlineLayout::panelCornerSize);

    g.setColour(t.panelBorder);
    g.drawRoundedRectangle(VoxlineLayout::inputPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::polishPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::outputPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::eqPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::dynamicsPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::spacePanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);

    // Input divider
    g.setColour(t.panelBorder);
    g.fillRect(VoxlineLayout::inputDividerBounds.toFloat());

    // Icons
    paintIcons(g);

    // LED dots
    paintLedDots(g, VoxlineLayout::inputLedDotsBounds);

    // Input panel upper section — split left/right
    g.setColour(t.textSecondary);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("AUTO GAIN", 293, 168, 100, 18, juce::Justification::centred, false);

    // Gain value
    g.setColour(t.textPrimary);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText(inputGainSlider.getTextFromValue(inputGainSlider.getValue()),
               VoxlineLayout::inputGainValueBounds, juce::Justification::centred, false);

    // Input level label + dB value — centered on right half
    g.setColour(t.textSecondary);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("INPUT LEVEL", 278, 260, 130, 18, juce::Justification::centred, false);
    g.setColour(t.textMuted);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("-18.4 dB", 293, 312, 100, 22, juce::Justification::centred, false);

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
    drawKnobLabel(VoxlineLayout::lowCutKnobBounds, "LOW CUT", "80 Hz");
    drawKnobLabel(VoxlineLayout::cleanKnobBounds, "CLEAN", "30%");
    drawKnobLabel(VoxlineLayout::deEssKnobBounds, "DE-ESS", "25%");

    // POLISH status + description
    {
        const auto val = polishSlider.getValue();
        const juce::String status = val < 36 ? "NATURAL" : (val < 71 ? "PUSHED" : "INTENSE");
        g.setColour(t.accentRose);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(status, VoxlineLayout::polishStatusBounds, juce::Justification::centred, false);
        g.setColour(t.textMuted);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("VOCAL FINISH MACRO", VoxlineLayout::polishDescBounds, juce::Justification::centred, false);
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

            drawMeter(VoxlineLayout::outMeterBounds, outLevel, outPeak, t.accentRose);
            drawMeter(VoxlineLayout::grMeterBounds,  grLevel,  grPeak,  t.accentLavender);
        }

        // -- Meter labels (below meters) --
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("OUT", 1126, 476, 50, 16, juce::Justification::centred, false);
        g.drawText("GR",  1206, 476, 40, 16, juce::Justification::centred, false);

        // -- Soft Clip pill button --
        {
            const auto r = VoxlineLayout::softClipBounds;
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
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawFittedText("OUTPUT GAIN", 1268, 245, 120, 20, juce::Justification::centred, 1);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(outputGainSlider.getTextFromValue(outputGainSlider.getValue()),
                   VoxlineLayout::outputGainValueBounds, juce::Justification::centred, false);
    }

    // Vocal EQ curve display
    {
        const auto cb = VoxlineLayout::eqCurveBounds.toFloat();
        const auto dark = (currentThemeIndex != 0);
        const auto& t = VoxlineTheme::get(currentThemeIndex);
        const auto cbI = cb.toNearestInt();

        // Dark background
        g.setColour(dark ? juce::Colour(0xff100E18) : juce::Colour(0xff282430));
        g.fillRoundedRectangle(cb, 8.0f);

        g.saveState();
        g.reduceClipRegion(cbI);

        const float logMin = std::log10(20.0f), logMax = std::log10(20000.0f);
        const float x0 = cb.getX() + 18, xW = cb.getWidth() - 36;
        const float yTop = cb.getY() + 14, yBot = cb.getBottom() - 14;
        const float yMid = cb.getCentreY();
        const float yScale = (cb.getHeight() - 28) / 24.0f;

        // Grid
        const auto gridMajor = juce::Colour(dark ? 0xff2A2638 : 0xff3A3648);
        const auto gridMinor = juce::Colour(dark ? 0xff1E1B2A : 0xff2E2A38);
        const float minorHz[] = { 20,30,40,50,60,80,100,200,300,400,500,600,800,1000,2000,3000,4000,5000,6000,8000,10000,20000 };
        for (auto f : minorHz)
        {
            const float x = x0 + xW * (std::log10(f) - logMin) / (logMax - logMin);
            const bool major = (f == 20 || f == 50 || f == 100 || f == 200 || f == 500 || f == 1000 || f == 2000 || f == 5000 || f == 10000 || f == 20000);
            g.setColour(major ? gridMajor : gridMinor);
            g.drawVerticalLine(juce::roundToInt(x), juce::roundToInt(yTop), juce::roundToInt(yBot));
        }
        for (int db = -12; db <= 12; db += 6)
        {
            const float y = yMid - (float)db * yScale;
            g.setColour(db == 0 ? gridMajor.brighter(0.3f) : gridMinor);
            g.drawHorizontalLine(juce::roundToInt(y), juce::roundToInt(x0 - 6), juce::roundToInt(x0 + xW + 6));
        }

        auto toX = [&](float hz) { return x0 + xW * (std::log10(juce::jlimit(20.0f, 20000.0f, hz)) - logMin) / (logMax - logMin); };
        auto toY = [&](float db) { return juce::jlimit(yTop, yBot, yMid - juce::jlimit(-12.0f, 12.0f, db) * yScale); };

        // White EQ curve
        juce::Path eqPath;
        const int steps = 140;
        for (int i = 0; i <= steps; ++i)
        {
            const float hz = 20.0f * std::pow(1000.0f, (float)i / (float)steps);
            float resp = 0.0f;
            if (hz < 80.0f) resp -= 24.0f * std::log2(80.0f / hz);
            { const float w = hz / 200.0f; resp += 2.0f / (1.0f + (w - 1.0f/w) * (w - 1.0f/w)); }
            { const float w = hz / 400.0f; resp -= 3.0f / (1.0f + ((w - 1.0f/w) / 1.5f) * ((w - 1.0f/w) / 1.5f)); }
            { const float w = hz / 2500.0f; resp += 3.0f / (1.0f + ((w - 1.0f/w) / 1.2f) * ((w - 1.0f/w) / 1.2f)); }
            { const float w = hz / 10000.0f; resp += 2.0f * (w * w / (w * w + 1.0f)); }
            if (hz > 18000.0f) resp -= 24.0f * std::log2(hz / 18000.0f);
            const float px = toX(hz), py = toY(resp);
            if (i == 0) eqPath.startNewSubPath(px, py);
            else        eqPath.lineTo(px, py);
        }
        g.setColour(juce::Colour(dark ? 0xffF0ECF8 : 0xffFAF8FF).withAlpha(0.12f));
        g.strokePath(eqPath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved));
        g.setColour(dark ? juce::Colour(0xffE8E4F0) : juce::Colour(0xffF5F2FA));
        g.strokePath(eqPath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));

        // Colored band nodes with selection glow
        struct BandDot { float hz; float db; int idx; juce::Colour col; };
        const BandDot dots[] = {
            { 80.0f,  -3.0f, 0, juce::Colour(dark ? 0xffA98CFF : 0xff8D70E8) },
            { 200.0f,  2.0f, 1, juce::Colour(dark ? 0xff80b080 : 0xff60a060) },
            { 400.0f, -3.0f, 2, juce::Colour(dark ? 0xffE6B45C : 0xffD8A548) },
            { 2500.0f, 3.0f, 3, juce::Colour(dark ? 0xffF2A766 : 0xffE99A5C) },
            { 10000.0f,2.0f, 4, juce::Colour(dark ? 0xff7BA4D8 : 0xff5B8EC0) },
            { 18000.0f,-3.0f,5, juce::Colour(dark ? 0xff9D96A8 : 0xff7E7888) },
        };
        for (auto& d : dots)
        {
            const auto cx = toX(d.hz), cy = toY(d.db);
            if (d.idx == selectedEqBand)
            {
                g.setColour(d.col.withAlpha(0.35f));
                g.fillEllipse(cx - 8, cy - 8, 16, 16);
                g.setColour(d.col.withAlpha(0.6f));
                g.drawEllipse(cx - 8.5f, cy - 8.5f, 17, 17, 1.5f);
            }
            g.setColour(d.col);
            g.fillEllipse(cx - 4, cy - 4, 8, 8);
        }

        // Frequency labels
        g.setColour(juce::Colour(dark ? 0xff6E6878 : 0xff8E8898));
        g.setFont(juce::FontOptions(8.0f));
        for (auto f : { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
        {
            const float x = toX((float)f);
            const juce::String label = (f >= 1000) ? juce::String(f / 1000) + "k" : juce::String(f);
            g.drawText(label, juce::roundToInt(x - 15), juce::roundToInt(yBot + 2), 30, 12, juce::Justification::centred, false);
        }

        g.restoreState();

        g.setColour(dark ? juce::Colour(0xff2A2635) : juce::Colour(0xff3A3645));
        g.drawRoundedRectangle(cb.reduced(0.5f), 8.0f, 1.0f);
    }

    // Selected band control row
    {
        const auto& t = VoxlineTheme::get(currentThemeIndex);
        const auto dark = (currentThemeIndex != 0);
        const char* bandNames[] = { "HPF", "LOW", "MUD", "PRES", "AIR", "LPF" };
        const char* freqVals[] = { "80 Hz", "160 Hz", "350 Hz", "2.5 kHz", "10 kHz", "18 kHz" };
        const char* gainVals[] = { "24 dB/oct", "+1.5 dB", "-2.0 dB", "+2.0 dB", "+1.5 dB", "12 dB/oct" };
        const juce::Colour bandCols[] = {
            juce::Colour(dark ? 0xffA98CFF : 0xff8D70E8),
            juce::Colour(dark ? 0xff80b080 : 0xff60a060),
            juce::Colour(dark ? 0xffE6B45C : 0xffD8A548),
            juce::Colour(dark ? 0xffF2A766 : 0xffE99A5C),
            juce::Colour(dark ? 0xff7BA4D8 : 0xff5B8EC0),
            juce::Colour(dark ? 0xff9D96A8 : 0xff7E7888),
        };
        const int sel = selectedEqBand;
        const bool isBell = (sel >= 1 && sel <= 4);

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
        g.drawText(freqVals[sel], VoxlineLayout::eqFreqValueBounds, juce::Justification::centred, false);

        // GAIN / SLOPE
        const char* gLabel = isBell ? "GAIN" : "SLOPE";
        g.setColour(t.textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(gLabel, VoxlineLayout::eqGainLabelBounds, juce::Justification::centred, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(gainVals[sel], VoxlineLayout::eqGainValueBounds, juce::Justification::centred, false);

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
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::resized()
{
    logoLabel.setBounds(VoxlineLayout::logoBounds);
    subtitleLabel.setBounds(VoxlineLayout::subtitleBounds);
    themeToggle->setBounds(VoxlineLayout::settingsButtonBounds);

    // === Top Bar ===
    presetDropdown.setBounds(VoxlineLayout::presetDropdownBounds);
    abButton.setBounds(VoxlineLayout::abButtonBounds);
    listenButton.setBounds(VoxlineLayout::listenUtilityBounds);
    bypassButton.setBounds(VoxlineLayout::bypassToggleBounds);

    // === Input Panel ===
    inputTitleLabel.setBounds(VoxlineLayout::inputTitleBounds);
    inputGainSlider.setBounds(VoxlineLayout::inputGainKnobBounds);
    lowCutKnob.setBounds(VoxlineLayout::lowCutKnobBounds);
    cleanKnob.setBounds(VoxlineLayout::cleanKnobBounds);
    deEssKnob.setBounds(VoxlineLayout::deEssKnobBounds);
    autoGainButton.setBounds(VoxlineLayout::autoGainToggleBounds);

    // === POLISH ===
    polishTitleLabel.setBounds(VoxlineLayout::polishTitleBounds);
    polishSlider.setBounds(VoxlineLayout::polishSliderBounds);

    // === Output ===
    outputTitleLabel.setBounds(VoxlineLayout::outputTitleBounds);
    outputMeter.setBounds(VoxlineLayout::outMeterBounds);
    gainReductionMeter.setBounds(VoxlineLayout::grMeterBounds);
    outputGainSlider.setBounds(VoxlineLayout::outputGainKnobBounds);

    // === Vocal EQ (placeholder: old tone knobs) ===
    bodySlider.setBounds(VoxlineLayout::eqLowBounds.withHeight(80).translated(0, 28));
    claritySlider.setBounds(VoxlineLayout::eqPresBounds.withHeight(80).translated(0, 28));
    airSlider.setBounds(VoxlineLayout::eqAirBounds.withHeight(80).translated(0, 28));
    smoothSlider.setBounds(VoxlineLayout::eqLpfBounds.withHeight(80).translated(0, 28));
    toneTitleLabel.setBounds(VoxlineLayout::eqTitleBounds);
    eqOnButton.setBounds(VoxlineLayout::eqOnToggleBounds);
    eqFreqKnob.setBounds(VoxlineLayout::eqFreqKnobBounds);
    eqGainKnob.setBounds(VoxlineLayout::eqGainKnobBounds);

    eqHpfButton.setBounds(VoxlineLayout::eqHpfBounds);
    eqLowButton.setBounds(VoxlineLayout::eqLowBounds);
    eqMudButton.setBounds(VoxlineLayout::eqMudBounds);
    eqPresButton.setBounds(VoxlineLayout::eqPresBounds);
    eqAirButton.setBounds(VoxlineLayout::eqAirBounds);
    eqLpfButton.setBounds(VoxlineLayout::eqLpfBounds);

    // === Dynamics ===
    meterNamesLabel.setBounds(VoxlineLayout::dynamicsTitleBounds);
    compSlider.setBounds(VoxlineLayout::compKnobBounds);
    thresholdKnob.setBounds(VoxlineLayout::thresholdKnobBounds);
    driveSlider.setBounds(VoxlineLayout::driveKnobBounds);
    ratioKnob.setBounds(VoxlineLayout::ratioKnobBounds);
    attackKnob.setBounds(VoxlineLayout::attackKnobBounds);
    releaseKnob.setBounds(VoxlineLayout::releaseKnobBounds);

    // === SPACE ===
    spaceTitleLabel.setBounds(VoxlineLayout::spaceTitleBounds);
    spaceTypeCombo.setBounds(VoxlineLayout::spaceTypeBounds);
    spaceSlider.setBounds(VoxlineLayout::spaceSliderBounds);
    spaceAmountLabel.setBounds(VoxlineLayout::spaceValueBounds);
    preDelayKnob.setBounds(VoxlineLayout::spacePreDelayKnobBounds);
    spaceHpfKnob.setBounds(VoxlineLayout::spaceHpfKnobBounds);
    spaceLpfKnob.setBounds(VoxlineLayout::spaceLpfKnobBounds);
    monitorLabel.setBounds(VoxlineLayout::monitorTitleBounds);
    monitorAbBtn.setBounds(VoxlineLayout::monitorAbBounds);
    monitorListenBtn.setBounds(VoxlineLayout::monitorListenBounds);
    monitorBypassBtn.setBounds(VoxlineLayout::monitorBypassBounds);

    // === Footer ===
    footerLabel.setBounds(VoxlineLayout::footerBounds);

    // === Placeholder labels ===
    thresholdLabel.setBounds(VoxlineLayout::thresholdKnobBounds);
    preDelayLabel.setBounds(VoxlineLayout::spacePreDelayKnobBounds);
    spaceHpfLabel.setBounds(VoxlineLayout::spaceHpfKnobBounds);
    spaceLpfLabel.setBounds(VoxlineLayout::spaceLpfKnobBounds);
    monitorLabel.setBounds(VoxlineLayout::monitorTitleBounds);
}

// ---------------------------------------------------------------------------
// Parameter listener
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == VoxlineParameterIDs::polish)
    {
        pendingPolishValue.store(newValue);
        triggerAsyncUpdate();
    }
    else if (parameterID == VoxlineParameterIDs::outputGain)
    {
        auto* param = audioProcessor.getAPVTS().getParameter(parameterID);
    }
    else if (parameterID == VoxlineParameterIDs::spaceAmount)
    {
        auto* param = audioProcessor.getAPVTS().getParameter(parameterID);
        if (param)
            spaceAmountLabel.setText(param->getCurrentValueAsText(), juce::dontSendNotification);
    }
    else if (parameterID == VoxlineParameterIDs::spaceType)
    {
        const int t = juce::roundToInt(newValue * 3.0f);
        const juce::String names[] = {"Tight", "Slap", "Wide"};
        spaceTypeCombo.setSelectedId(t + 1, juce::dontSendNotification);
    }
    else if (parameterID == VoxlineParameterIDs::autoGain)
    {
        autoGainButton.setButtonText(newValue >= 0.5f ? "ON" : "OFF");
    }

    // Update active A/B slot on every parameter change
    if (!applyingSnapshot)
        captureSnapshot(isSlotAActive ? snapshotA : snapshotB);
}

void VoxlineAudioProcessorEditor::handleAsyncUpdate()
{
    DBG("VOXLINE polish changed: " + juce::String(pendingPolishValue.load(), 2));
    repaint();
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------
bool VoxlineAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    const auto k = key.getTextCharacter();

    if (k == 't' || k == 'T') { cycleTheme(); return true; }

    return false;
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::applyTheme(const VoxlineTheme& theme, int index)
{
    currentThemeIndex = index;
    const auto dark = (index != 0);

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
    outputGainSlider.setTheme(theme);
    lowCutKnob.setTheme(theme);
    cleanKnob.setTheme(theme);
    deEssKnob.setTheme(theme);

    // Labels — logo + subtitle
    logoLabel.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    subtitleLabel.setFont(juce::FontOptions(13.0f));

    // Panel titles — unified: 14px bold, subtle tracking, Text Primary
    const auto titleFont = juce::Font(14.0f, juce::Font::bold).withExtraKerningFactor(0.06f);
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
    const auto inactiveBg = dark ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfffaf7f2);
    abButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    abButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);

    // Monitor buttons — same style but quieter
    auto styleMonitorBtn = [&](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonColourId, inactiveBg);
        b.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
    };
    styleMonitorBtn(monitorAbBtn);
    styleMonitorBtn(monitorListenBtn);
    styleMonitorBtn(monitorBypassBtn);

    // === SPACE ===
    VoxlineSpaceSliderLNF::spaceSliderTheme = index;
    spaceSlider.repaint();

    // === Preset dropdown ===
    VoxlinePresetDropdownLNF::currentDropdownTheme = index;

    // === Vocal EQ band buttons (single-theme PNGs, no theme update needed) ===
    VoxlineAutoGainLNF::currentAutoGainTheme = index;
    presetDropdown.getProperties().set("themeIndex", index);
    presetDropdown.repaint();

    // === Toggles ===
    autoGainButton.setColour(juce::ToggleButton::textColourId, 
        dark ? juce::Colour(0xff9d99a8) : juce::Colour(0xff666666));
    cleanModeButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);
    cleanModeButton.setColour(juce::ToggleButton::tickColourId, theme.accentLavender);
    // listen / eqOn — VoxlineImageButton (theme-aware), already updated via setThemeIndex above
    eqOnButton.setThemeIndex(index);

    // === Meters ===
    const auto meterWell = dark ? juce::Colour(0xff14121A) : juce::Colour(0xffD5CFC8);
    outputMeter.setColour(VoxlineLevelMeter::backgroundColour, meterWell);
    outputMeter.setColour(VoxlineLevelMeter::foregroundColour, theme.meterMid);
    gainReductionMeter.setColour(VoxlineLevelMeter::backgroundColour, meterWell);
    gainReductionMeter.setColour(VoxlineLevelMeter::foregroundColour, theme.meterLow);

    // === Icons ===
    loadIconDrawables(dark);

    repaint();
}

void VoxlineAudioProcessorEditor::cycleTheme()
{
    const auto nextIndex = (currentThemeIndex + 1) % 2;
    applyTheme(VoxlineTheme::get(nextIndex), nextIndex);
    DBG("VOXLINE theme: " << juce::String(nextIndex == 0 ? "Light" : "Dark"));
}

// ---------------------------------------------------------------------------
// Icons
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::loadIconDrawables(bool dark)
{
    const auto parse = [](const char* data, int size) -> std::unique_ptr<juce::Drawable>
    {
        if (!data || size <= 0) return nullptr;
        auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, size));
        return xml ? juce::Drawable::createFromSVG(*xml) : nullptr;
    };

    if (dark)
    {
        cachedBypassIcon  = parse(BinaryData::bypass_dark_svg,  BinaryData::bypass_dark_svgSize);
        cachedListenIcon  = parse(BinaryData::listen_dark_svg,  BinaryData::listen_dark_svgSize);
        cachedSettingsIcon = parse(BinaryData::settings_dark_svg, BinaryData::settings_dark_svgSize);
    }
    else
    {
        cachedBypassIcon  = parse(BinaryData::bypass_light_svg,  BinaryData::bypass_light_svgSize);
        cachedListenIcon  = parse(BinaryData::listen_light_svg,  BinaryData::listen_light_svgSize);
        cachedSettingsIcon = parse(BinaryData::settings_light_svg, BinaryData::settings_light_svgSize);
    }
}

void VoxlineAudioProcessorEditor::paintIcons(juce::Graphics& g)
{
    const auto draw = [&](juce::Drawable* d, int x, int y, int w, int h)
    {
        if (d)
            d->drawWithin(g, juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h),
                          juce::RectanglePlacement::centred, 1.0f);
    };

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
    const float cy = bounds.getCentreY();

    const int activeDots = juce::jlimit(0, numDots, juce::roundToInt(inputLedLevel * (float)numDots));

    for (int i = 0; i < numDots; ++i)
    {
        const float cx = bounds.getX() + (float)i * spacing;
        if (i < activeDots)
        {
            // Gradient from green-ish (low) to accentLavender (high)
            const auto t = (float)i / (float)(numDots - 1);
            g.setColour(juce::Colour::fromFloatRGBA(0.55f + t * 0.13f, 0.72f + t * 0.11f, 0.55f + t * 0.45f, 1.0f));
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
    const auto inRms = proc.inputRms.load();
    const auto outPeak = proc.outputPeak.load();
    const auto outRms = proc.outputRms.load();
    const auto gr = proc.gainReduction.load();

    inputLedLevel = inPeak;

    outputMeter.setLevel(outPeak);
    gainReductionMeter.setLevel(gr);

    // Update PEAK/RMS readout
    const auto outPeakDb = juce::Decibels::gainToDecibels(juce::jmax(outPeak, 0.00001f), -60.0f);
    const auto outRmsDb = juce::Decibels::gainToDecibels(juce::jmax(outRms, 0.00001f), -60.0f);

    repaint();
}

// ---------------------------------------------------------------------------
// Preset system
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &abButton) { toggleAb(); return; }

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
            syncEQKnobsToSelectedBand();
            repaint();
            return;
        }
    }
}

void VoxlineAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetDropdown)
        applyPreset(presetDropdown.getText());
    else if (comboBoxThatHasChanged == &spaceTypeCombo)
    {
        const int t = spaceTypeCombo.getSelectedId() - 1;
        if (auto* p = audioProcessor.getAPVTS().getParameter(VoxlineParameterIDs::spaceType))
            p->setValueNotifyingHost((float)t / 2.0f);
    }
}

void VoxlineAudioProcessorEditor::applyPreset(const juce::String& name)
{
    auto& apvts = audioProcessor.getAPVTS();

    // Map name to dropdown ID
    static const std::vector<juce::String> presetIds = {"Clean","Basement Take","Dirty Lead","Cold Plug","Rage Cut","Muddy Trap","Cyber Vox","Noir Vocal","Tape Rap"};

    // Find preset index
    int idx = -1;
    for (int i = 0; i < (int)presetIds.size(); ++i)
    {
        if (presetIds[i] == name) { idx = i; break; }
    }
    if (idx < 0) return;

    // Preset table: { inGain, autoGain, polish, body, clarity, air, smooth, comp, drive, outGain }
    struct PresetDef { float in; bool ag; float pol, bd, cl, ar, sm, cp, dr, out; float spAmt; int spType; };
    static const PresetDef presets[] = {
        // name,            inG,  ag,  pol, bd,  cl,  ar,  sm,  cp,  dr,  out,     spAmt, spType
        {  0.0f, true,  22,  50,  42,  30,  10,  18,   0,  0.0f,     0, 0 }, // Clean
        { -1.0f, true,  58,  68,  54,  32,  20,  52,  34, -1.5f,    12, 1 }, // Basement Take
        { -1.5f, true,  78,  62,  78,  48,  24,  76,  46, -2.0f,     8, 0 }, // Dirty Lead
        { -1.0f, true,  68,  30,  64,  82,  66,  48,  10, -1.5f,    20, 2 }, // Cold Plug
        { -2.0f, true,  86,  38,  90,  72,  22,  84,  56, -3.0f,    10, 2 }, // Rage Cut
        { -1.5f, true,  72,  84,  52,  24,  28,  70,  48, -2.5f,     6, 1 }, // Muddy Trap
        { -2.0f, true,  88,  24,  86,  94,  38,  78,  32, -3.0f,    25, 0 }, // Cyber Vox
        { -1.0f, true,  60,  58,  44,  26,  70,  46,  18, -1.5f,    18, 1 }, // Noir Vocal
        { -1.5f, true,  70,  72,  56,  36,  38,  66,  58, -2.5f,    12, 2 }, // Tape Rap
    };
    auto& p = presets[idx];

    auto setParam = [&](const juce::String& id, float value) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(value);
    };

    setParam("cleanMode",  0.0f);
    setParam("inputGain",  (p.in  + 24.0f) / 48.0f);
    setParam("autoGain",   p.ag ? 1.0f : 0.0f);
    setParam("polish",     p.pol / 100.0f);
    setParam("body",       p.bd  / 100.0f);
    setParam("clarity",    p.cl  / 100.0f);
    setParam("air",        p.ar  / 100.0f);
    setParam("smooth",     p.sm  / 100.0f);
    setParam("comp",       p.cp  / 100.0f);
    setParam("drive",      p.dr  / 100.0f);
    setParam("outputGain", (p.out + 24.0f) / 48.0f);
    setParam("spaceAmount", p.spAmt / 100.0f);
    setParam("spaceType",   (float)p.spType / 3.0f);

    // Sync dropdown
    presetDropdown.setSelectedId(idx + 1, juce::dontSendNotification);

    DBG("VOXLINE preset: " << name);
}

void VoxlineAudioProcessorEditor::captureSnapshot(ParameterSnapshot& snap)
{
    auto& apvts = audioProcessor.getAPVTS();
    auto val = [&](const juce::String& id) -> float {
        if (auto* p = apvts.getParameter(id)) return p->getValue();
        return 0.0f;
    };
    snap.inputGain  = val("inputGain");
    snap.autoGain   = val("autoGain") >= 0.5f;
    snap.polish     = val("polish");
    snap.body       = val("body");
    snap.clarity    = val("clarity");
    snap.air        = val("air");
    snap.smooth     = val("smooth");
    snap.comp       = val("comp");
    snap.drive      = val("drive");
    snap.outputGain = val("outputGain");
    snap.spaceAmount = val("spaceAmount");
    snap.spaceType   = val("spaceType");
    snap.cleanMode  = val("cleanMode") >= 0.5f;
    snap.bypass     = val("bypass") >= 0.5f;
    snap.listen     = val("listen") >= 0.5f;
}

void VoxlineAudioProcessorEditor::applySnapshot(const ParameterSnapshot& snap)
{
    auto& apvts = audioProcessor.getAPVTS();
    auto set = [&](const juce::String& id, float v) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(v);
    };
    set("inputGain",  snap.inputGain);
    set("autoGain",   snap.autoGain ? 1.0f : 0.0f);
    set("polish",     snap.polish);
    set("body",       snap.body);
    set("clarity",    snap.clarity);
    set("air",        snap.air);
    set("smooth",     snap.smooth);
    set("comp",       snap.comp);
    set("drive",      snap.drive);
    set("outputGain", snap.outputGain);
    set("spaceAmount", snap.spaceAmount);
    set("spaceType",   snap.spaceType);
    set("cleanMode",  snap.cleanMode ? 1.0f : 0.0f);
    set("bypass",     snap.bypass ? 1.0f : 0.0f);
    set("listen",     snap.listen ? 1.0f : 0.0f);
}

void VoxlineAudioProcessorEditor::toggleAb()
{
    // Save current state to active slot, then switch
    captureSnapshot(isSlotAActive ? snapshotA : snapshotB);
    isSlotAActive = !isSlotAActive;

    applyingSnapshot = true;
    applySnapshot(isSlotAActive ? snapshotA : snapshotB);
    applyingSnapshot = false;

    // Update button visual
    auto& t = VoxlineTheme::get(currentThemeIndex);
    abButton.setButtonText(isSlotAActive ? "A" : "B");
    const auto activeBg = t.accentRose.withAlpha(0.22f);
    abButton.setColour(juce::TextButton::buttonColourId,
                       isSlotAActive ? activeBg : (t.editorBg.getBrightness() < 0.3f ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfffaf7f2)));
    abButton.setColour(juce::TextButton::textColourOffId,
                       isSlotAActive ? t.accentRose : t.textPrimary);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::configureKnob(VoxlineCustomKnob& knob)           { addAndMakeVisible(knob); }
void VoxlineAudioProcessorEditor::configureButton(juce::ToggleButton& b, const juce::String& t) { b.setButtonText(t); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configureHeaderButton(juce::TextButton& b, const juce::String& t) { b.setButtonText(t); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configurePresetButton(juce::TextButton& b, const juce::String& t, bool) { b.setButtonText(t); b.setEnabled(true); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configureTextLabel(juce::Label& l, const juce::String& t, juce::Justification j) { l.setText(t, juce::dontSendNotification); l.setJustificationType(j); addAndMakeVisible(l); }

void VoxlineAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    auto& apvts = audioProcessor.getAPVTS();
    if (slider == &eqFreqKnob || slider == &eqGainKnob)
    {
        const int sel = selectedEqBand;
        const float val = (float)slider->getValue();

        const char* freqIDs[] = { VoxlineParameterIDs::hpfFreq, VoxlineParameterIDs::lowFreq, VoxlineParameterIDs::mudFreq, VoxlineParameterIDs::presFreq, VoxlineParameterIDs::airFreq, VoxlineParameterIDs::lpfFreq };
        const char* gainIDs[] = { VoxlineParameterIDs::hpfSlope, VoxlineParameterIDs::lowGain, VoxlineParameterIDs::mudGain, VoxlineParameterIDs::presGain, VoxlineParameterIDs::airGain, VoxlineParameterIDs::lpfSlope };

        if (slider == &eqFreqKnob)
        {
            if (auto* p = apvts.getParameter(freqIDs[sel]))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        }
        else
        {
            if (auto* p = apvts.getParameter(gainIDs[sel]))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        }
        repaint();
    }
}

void VoxlineAudioProcessorEditor::syncEQKnobsToSelectedBand()
{
    auto& apvts = audioProcessor.getAPVTS();
    const int sel = selectedEqBand;

    const char* freqIDs[] = { VoxlineParameterIDs::hpfFreq, VoxlineParameterIDs::lowFreq, VoxlineParameterIDs::mudFreq, VoxlineParameterIDs::presFreq, VoxlineParameterIDs::airFreq, VoxlineParameterIDs::lpfFreq };
    const char* gainIDs[] = { VoxlineParameterIDs::hpfSlope, VoxlineParameterIDs::lowGain, VoxlineParameterIDs::mudGain, VoxlineParameterIDs::presGain, VoxlineParameterIDs::airGain, VoxlineParameterIDs::lpfSlope };
    const float freqDefaults[] = { 80.0f, 160.0f, 350.0f, 2500.0f, 10000.0f, 18000.0f };
    const float gainDefaults[] = { 1.0f, 1.5f, -2.0f, 2.0f, 1.5f, 0.0f }; // hpfSlope=24(1), lpfSlope=12(0)

    if (auto* p = apvts.getParameter(freqIDs[sel]))
    {
        eqFreqKnob.setRange(p->getNormalisableRange().start, p->getNormalisableRange().end, p->getNormalisableRange().interval);
        eqFreqKnob.setValue(p->getValue(), juce::dontSendNotification);
    }
    if (auto* p = apvts.getParameter(gainIDs[sel]))
    {
        eqGainKnob.setRange(p->getNormalisableRange().start, p->getNormalisableRange().end, p->getNormalisableRange().interval);
        eqGainKnob.setValue(p->getValue(), juce::dontSendNotification);
    }
}

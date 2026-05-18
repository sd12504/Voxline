#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/Layout.h"

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
    configureTextLabel(peakRmsLabel, "PEAK -3.4\nRMS -14.8", juce::Justification::centred);
    configureTextLabel(meterNamesLabel, "DYNAMICS / COLOR", juce::Justification::centred);

    // EQ band buttons
    auto addEqBand = [&](juce::TextButton& b, const juce::String& t) {
        b.setButtonText(t);
        b.setEnabled(true);
        addAndMakeVisible(b);
    };
    addEqBand(eqHpfButton, "HPF");
    addEqBand(eqLowButton, "LOW");
    addEqBand(eqMudButton, "MUD");
    addEqBand(eqPresButton, "PRES");
    addEqBand(eqAirButton, "AIR");
    addEqBand(eqLpfButton, "LPF");

    // EQ band button highlights — LOW active by default
    eqLowButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa0c0a0).withAlpha(0.25f));

    // Placeholder controls (UI only, no DSP yet)
    configureTextLabel(peakLabel, "PEAK", juce::Justification::centredLeft);
    configureTextLabel(rmsLabel, "RMS", juce::Justification::centredLeft);
    configureTextLabel(outLabel, "OUT", juce::Justification::centred);
    configureTextLabel(grLabel, "GR", juce::Justification::centred);
    configureTextLabel(thresholdLabel, "THRESHOLD\n-18.0 dB", juce::Justification::centred);
    configureTextLabel(preDelayLabel, "PRE-DELAY\n15 ms", juce::Justification::centred);
    configureTextLabel(spaceHpfLabel, "HPF\n200 Hz", juce::Justification::centred);
    configureTextLabel(spaceLpfLabel, "LPF\n8.0 kHz", juce::Justification::centred);
    configureTextLabel(softClipLabel, "SOFT CLIP", juce::Justification::centred);
    configureTextLabel(monitorLabel, "MONITOR", juce::Justification::centredLeft);

    configurePresetButton(abButton, "A/B");

    abButton.addListener(this);

    configureKnob(inputGainSlider);
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
    configureKnob(polishSlider);
    configureKnob(bodySlider);
    configureKnob(claritySlider);
    configureKnob(airSlider);
    configureKnob(smoothSlider);
    configureKnob(compSlider);
    configureKnob(driveSlider);
    // Output knob: no internal text, full 70x70 for circle
    configureKnob(outputGainSlider);
    outputGainSlider.setShowInternalLabel(false);
    outputGainSlider.setShowInternalValue(false);

    // SPACE control in bottom bar
    spaceTypeCombo.addItemList({"Tight Ambience", "Filtered Slap", "Stereo Wide"}, 1);
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

    configureTextLabel(spaceAmountLabel, "0%", juce::Justification::centredRight);

    // Footer
    configureTextLabel(footerLabel, "VOXLINE 2.0.0  |  SADTONY", juce::Justification::centred);


    configureTextLabel(outputValueLabel, "0.0 dB", juce::Justification::centred);

    configureButton(bypassButton, " BYPASS");
    bypassButton.setLookAndFeel(&voxlineToggleLNF);
    configureButton(autoGainButton, "Auto Gain");
    autoGainButton.setLookAndFeel(&voxlineAutoGainLNF);
    configureButton(cleanModeButton, "Clean");
    cleanModeButton.setLookAndFeel(&voxlineToggleLNF);
    configureButton(listenButton, "  Listen");
    listenButton.setLookAndFeel(&voxlineToggleLNF);
    // Listen mode: difference monitor (solo wet signal) — standard audition behavior

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
    spaceAttachment = std::make_unique<SliderAttachment>(apvts, VoxlineParameterIDs::spaceAmount, spaceSlider);

    autoGainAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::autoGain, autoGainButton);
    bypassAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::bypass, bypassButton);
    cleanModeAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::cleanMode, cleanModeButton);
    listenAttachment = std::make_unique<ButtonAttachment>(apvts, VoxlineParameterIDs::listen, listenButton);

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
    bypassButton.setLookAndFeel(nullptr);
    listenButton.setLookAndFeel(nullptr);
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

    // Input panel knob labels (drawn externally because knobs are too small)
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    auto drawKnobLabel = [&](juce::Rectangle<int> r, const juce::String& label, const juce::String& value) {
        g.setColour(t.textSecondary);
        g.drawText(label, r.getX(), r.getY() - 36, r.getWidth(), 16, juce::Justification::centred, false);
        g.setColour(t.textPrimary);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(value, r.getX(), r.getBottom() + 2, r.getWidth(), 18, juce::Justification::centred, false);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    };
    drawKnobLabel(VoxlineLayout::lowCutKnobBounds, "LOW CUT", "80 Hz");
    drawKnobLabel(VoxlineLayout::cleanKnobBounds, "CLEAN", "30%");
    drawKnobLabel(VoxlineLayout::deEssKnobBounds, "DE-ESS", "25%");

    // Panel titles that don't have dedicated labels
    g.setColour(t.textPrimary);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("SPACE", VoxlineLayout::spaceTitleBounds, juce::Justification::centredLeft, false);

    // POLISH status + description
    {
        const auto val = polishSlider.getValue();
        const juce::String status = val < 36 ? "NATURAL" : (val < 71 ? "PUSHED" : "INTENSE");
        g.setColour(t.accentRose);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(status, VoxlineLayout::polishStatusBounds, juce::Justification::centred, false);
        g.setColour(t.textMuted);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("Core vocal processing macro — does not affect SPACE", VoxlineLayout::polishDescBounds, juce::Justification::centred, false);
    }

    // EQ curve display
    {
        auto curveBounds = VoxlineLayout::eqCurveBounds.toFloat();
        const auto dark = (currentThemeIndex != 0);
        g.setColour(dark ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfff0eae0));
        g.fillRoundedRectangle(curveBounds, 8.0f);
        g.setColour(t.panelBorder);
        g.drawRoundedRectangle(curveBounds, 8.0f, 1.0f);
        // Frequency grid lines
        g.setColour(t.panelBorder.withAlpha(0.4f));
        for (int i = 0; i < 5; ++i)
            g.drawVerticalLine(juce::roundToInt(curveBounds.getX() + curveBounds.getWidth() * (0.15f + 0.18f * i)),
                               curveBounds.getY() + 12, curveBounds.getBottom() - 12);
        g.drawHorizontalLine(juce::roundToInt(curveBounds.getCentreY()), curveBounds.getX() + 12, curveBounds.getRight() - 12);
        // EQ curve
        g.setColour(t.accentRose.withAlpha(0.5f));
        juce::Path curve;
        curve.startNewSubPath(curveBounds.getX() + 20, curveBounds.getCentreY() + 15);
        curve.lineTo(curveBounds.getCentreX() - 40, curveBounds.getCentreY() + 5);
        curve.lineTo(curveBounds.getCentreX(), curveBounds.getCentreY() - 8);
        curve.lineTo(curveBounds.getCentreX() + 60, curveBounds.getCentreY() + 3);
        curve.lineTo(curveBounds.getRight() - 20, curveBounds.getCentreY() + 10);
        g.strokePath(curve, juce::PathStrokeType(2.0f));
        // Band nodes
        for (auto& pt : { juce::Point<float>(curveBounds.getX() + 60, curveBounds.getCentreY() + 10),
                          juce::Point<float>(curveBounds.getCentreX() - 40, curveBounds.getCentreY() + 5),
                          juce::Point<float>(curveBounds.getCentreX(), curveBounds.getCentreY() - 8),
                          juce::Point<float>(curveBounds.getCentreX() + 60, curveBounds.getCentreY() + 3),
                          juce::Point<float>(curveBounds.getRight() - 70, curveBounds.getCentreY() + 8) })
        {
            g.setColour(t.accentRose);
            g.fillEllipse(pt.x - 3, pt.y - 3, 6, 6);
        }
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
    peakRmsLabel.setBounds(VoxlineLayout::peakValueBounds);
    outputMeter.setBounds(VoxlineLayout::outMeterBounds);
    gainReductionMeter.setBounds(VoxlineLayout::grMeterBounds);
    outputGainSlider.setBounds(VoxlineLayout::outputGainKnobBounds);
    outputValueLabel.setBounds(VoxlineLayout::outputGainValueBounds);

    // === Vocal EQ (placeholder: old tone knobs) ===
    bodySlider.setBounds(VoxlineLayout::eqLowBounds.withHeight(80).translated(0, 28));
    claritySlider.setBounds(VoxlineLayout::eqPresBounds.withHeight(80).translated(0, 28));
    airSlider.setBounds(VoxlineLayout::eqAirBounds.withHeight(80).translated(0, 28));
    smoothSlider.setBounds(VoxlineLayout::eqLpfBounds.withHeight(80).translated(0, 28));
    toneTitleLabel.setBounds(VoxlineLayout::eqTitleBounds);

    eqHpfButton.setBounds(VoxlineLayout::eqHpfBounds);
    eqLowButton.setBounds(VoxlineLayout::eqLowBounds);
    eqMudButton.setBounds(VoxlineLayout::eqMudBounds);
    eqPresButton.setBounds(VoxlineLayout::eqPresBounds);
    eqAirButton.setBounds(VoxlineLayout::eqAirBounds);
    eqLpfButton.setBounds(VoxlineLayout::eqLpfBounds);

    // === Dynamics (placeholder) ===
    meterNamesLabel.setBounds(VoxlineLayout::dynamicsTitleBounds);
    compSlider.setBounds(VoxlineLayout::compKnobBounds);
    driveSlider.setBounds(VoxlineLayout::driveKnobBounds);

    // === SPACE ===
    spaceTypeCombo.setBounds(VoxlineLayout::spaceTypeBounds);
    spaceSlider.setBounds(VoxlineLayout::spaceSliderBounds.withHeight(28).translated(0, 10));
    spaceAmountLabel.setBounds(VoxlineLayout::spaceValueBounds.translated(-10, 0));

    // === Footer ===
    footerLabel.setBounds(VoxlineLayout::footerBounds);

    // === Placeholder labels ===
    peakLabel.setBounds(VoxlineLayout::peakLabelBounds);
    rmsLabel.setBounds(VoxlineLayout::rmsLabelBounds);
    outLabel.setBounds(VoxlineLayout::outMeterBounds.getX() - 35, 168, 50, 20);
    grLabel.setBounds(VoxlineLayout::grMeterBounds.getX() - 5, 168, 50, 20);
    thresholdLabel.setBounds(VoxlineLayout::thresholdKnobBounds);
    softClipLabel.setBounds(VoxlineLayout::softClipBounds);
    preDelayLabel.setBounds(VoxlineLayout::spacePreDelayBounds);
    spaceHpfLabel.setBounds(VoxlineLayout::spaceHpfBounds);
    spaceLpfLabel.setBounds(VoxlineLayout::spaceLpfBounds);
    monitorLabel.setBounds(995, 848, 120, 20);
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
        if (param)
            outputValueLabel.setText(param->getCurrentValueAsText(), juce::dontSendNotification);
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
        const juce::String names[] = {"Tight Ambience", "Filtered Slap", "Stereo Wide", "Vocal Space"};
        spaceTypeCombo.setSelectedId(t + 1, juce::dontSendNotification);
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
    outputGainSlider.setTheme(theme);
    lowCutKnob.setTheme(theme);
    cleanKnob.setTheme(theme);
    deEssKnob.setTheme(theme);

    // Labels — fonts sizes for output panel
    logoLabel.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    subtitleLabel.setFont(juce::FontOptions(13.0f));
    peakRmsLabel.setFont(juce::FontOptions(12.0f));
    meterNamesLabel.setFont(juce::FontOptions(11.0f));
    outputValueLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    const auto setTextColour = [&](juce::Label& l) { l.setColour(juce::Label::textColourId, theme.textPrimary); };
    setTextColour(logoLabel);
    setTextColour(subtitleLabel);
    setTextColour(inputTitleLabel);
    setTextColour(toneTitleLabel);
    setTextColour(polishTitleLabel);
    setTextColour(outputTitleLabel);
    setTextColour(peakRmsLabel);
    setTextColour(meterNamesLabel);
    setTextColour(outputValueLabel);
    setTextColour(spaceAmountLabel);
    setTextColour(peakLabel);
    setTextColour(rmsLabel);
    setTextColour(outLabel);
    setTextColour(grLabel);
    setTextColour(thresholdLabel);
    setTextColour(softClipLabel);
    setTextColour(preDelayLabel);
    setTextColour(spaceHpfLabel);
    setTextColour(spaceLpfLabel);
    setTextColour(monitorLabel);

    // Placeholder label fonts
    auto stylePH = [&](juce::Label& l) { l.setFont(juce::FontOptions(10.0f)); };
    stylePH(thresholdLabel); stylePH(softClipLabel);
    stylePH(preDelayLabel); stylePH(spaceHpfLabel); stylePH(spaceLpfLabel);

    // Footer
    footerLabel.setFont(juce::FontOptions(10.0f));
    footerLabel.setColour(juce::Label::textColourId, theme.textMuted.withAlpha(0.5f));

    // === Header ===
    // Bypass text colour (checkbox hidden by LookAndFeel)
    bypassButton.setColour(juce::ToggleButton::textColourId, theme.textPrimary);
    bypassButton.setColour(juce::ToggleButton::textColourId, theme.textPrimary);
    bypassButton.setColour(juce::ToggleButton::tickColourId, theme.accentRose);

    // === Bottom bar utility buttons ===
    const auto inactiveBg = dark ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfffaf7f2);
    abButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    abButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);

    // === SPACE ===
    VoxlineSpaceSliderLNF::spaceSliderTheme = index;
    spaceSlider.repaint();

    // === Preset dropdown ===
    VoxlinePresetDropdownLNF::currentDropdownTheme = index;

    // === Vocal EQ band buttons ===
    auto styleEqBand = [&](juce::TextButton& b, juce::Colour c) {
        b.setColour(juce::TextButton::buttonColourId, c.withAlpha(dark ? 0.15f : 0.10f));
        b.setColour(juce::TextButton::textColourOffId, c);
    };
    styleEqBand(eqHpfButton, theme.accentPurple);
    styleEqBand(eqLowButton, juce::Colour(dark ? 0xff80b080 : 0xff60a060));
    styleEqBand(eqMudButton, juce::Colour(dark ? 0xffE6B45C : 0xffD8A548));
    styleEqBand(eqPresButton, juce::Colour(dark ? 0xffF2A766 : 0xffE99A5C));
    styleEqBand(eqAirButton, theme.accentLavender);
    styleEqBand(eqLpfButton, theme.accentPurple);
    VoxlineAutoGainLNF::currentAutoGainTheme = index;
    presetDropdown.getProperties().set("themeIndex", index);
    presetDropdown.repaint();

    // === Toggles ===
    autoGainButton.setColour(juce::ToggleButton::textColourId, 
        dark ? juce::Colour(0xff9d99a8) : juce::Colour(0xff666666));
    cleanModeButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);
    cleanModeButton.setColour(juce::ToggleButton::tickColourId, theme.accentLavender);
    listenButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);
    listenButton.setColour(juce::ToggleButton::tickColourId, theme.accentLavender);

    // === Meters ===
    outputMeter.setColour(VoxlineLevelMeter::backgroundColour, theme.panelBorder);
    outputMeter.setColour(VoxlineLevelMeter::foregroundColour, theme.meterMid);
    gainReductionMeter.setColour(VoxlineLevelMeter::backgroundColour, theme.panelBorder);
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
    peakRmsLabel.setText("PEAK " + juce::String(outPeakDb, 1) + "\nRMS " + juce::String(outRmsDb, 1), juce::dontSendNotification);

    repaint();
}

// ---------------------------------------------------------------------------
// Preset system
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &abButton)  toggleAb();
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

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
        // Draw only text — icon is drawn separately in paintIcons.
        // Offset text right past the icon area (24px) so they don't overlap.
        auto textArea = button.getLocalBounds().withTrimmedLeft(24);
        auto font = juce::FontOptions(12.0f);
        g.setFont(font);
        const auto on = button.getToggleState();
        g.setColour(button.findColour(on ? juce::ToggleButton::tickColourId
                                         : juce::ToggleButton::textColourId));
        g.drawText(button.getButtonText(), textArea, juce::Justification::centredLeft, false);
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
    configureTextLabel(subtitleLabel, "Fast Vocal Channel", juce::Justification::centredLeft);

    // Theme toggle icon
    themeToggle = std::make_unique<ThemeToggleComp>(*this);
    addAndMakeVisible(*themeToggle);

    // Preset dropdown in bottom bar
    presetDropdown.setLookAndFeel(&voxlineDropdownLNF);
    presetDropdown.setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    presetDropdown.addItemList({"Clean","Westwood","2Hollis","Yeat","Ken Carson","Bladee","Travis Scott","The Weeknd","Billie Eilish"}, 1);
    presetDropdown.setSelectedId(1, juce::dontSendNotification); // Clean
    presetDropdown.getProperties().set("themeIndex", 0);
    presetDropdown.addListener(this);
    addAndMakeVisible(presetDropdown);

    configureTextLabel(inputTitleLabel, "INPUT", juce::Justification::centred);
    configureTextLabel(toneTitleLabel, "TONE CONTROLS", juce::Justification::centred);
    configureTextLabel(polishTitleLabel, "POLISH", juce::Justification::centred);
    configureTextLabel(outputTitleLabel, "OUTPUT", juce::Justification::centred);
    configureTextLabel(peakRmsLabel, "PEAK -3.4\nRMS -14.8", juce::Justification::centred);
    configureTextLabel(meterNamesLabel, "OUT   GR", juce::Justification::centred);

    configurePresetButton(abButton, "A/B");

    abButton.addListener(this);

    configureKnob(inputGainSlider);
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

    configureTextLabel(outputValueLabel, "0.0 dB", juce::Justification::centred);

    configureButton(bypassButton, " BYPASS");
    bypassButton.setLookAndFeel(&voxlineToggleLNF);
    configureButton(autoGainButton, "Auto Gain");
    autoGainButton.setLookAndFeel(&voxlineAutoGainLNF);
    configureButton(listenButton, "  Listen");
    listenButton.setLookAndFeel(&voxlineToggleLNF);

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

    // Editor background
    g.fillAll(t.editorBg);

    // Main card
    g.setColour(t.mainCardBg);
    g.fillRoundedRectangle(VoxlineLayout::mainCard.toFloat(), VoxlineLayout::mainCornerSize);

    // Panel fills
    g.setColour(t.panelBg);
    g.fillRoundedRectangle(VoxlineLayout::inputPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::tonePanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::polishPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::outputPanel.toFloat(), VoxlineLayout::panelCornerSize);
    g.fillRoundedRectangle(VoxlineLayout::presetBar.toFloat(), VoxlineLayout::presetBarCornerSize);

    // Panel borders
    g.setColour(t.panelBorder);
    g.drawRoundedRectangle(VoxlineLayout::inputPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::tonePanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::polishPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::outputPanel.toFloat(), VoxlineLayout::panelCornerSize, 1.0f);
    g.drawRoundedRectangle(VoxlineLayout::presetBar.toFloat(), VoxlineLayout::presetBarCornerSize, 1.0f);

    // Icons
    paintIcons(g);

    // LED dots
    paintLedDots(g, VoxlineLayout::inputLedDotsBounds);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::resized()
{
    logoLabel.setBounds(VoxlineLayout::logoBounds);
    subtitleLabel.setBounds(VoxlineLayout::subtitleBounds);
    bypassButton.setBounds(VoxlineLayout::bypassButtonBounds);
    themeToggle->setBounds(VoxlineLayout::settingsButtonBounds);

    inputTitleLabel.setBounds(VoxlineLayout::inputTitleBounds);
    inputGainSlider.setBounds(VoxlineLayout::inputGainSliderBounds);
    autoGainButton.setBounds(VoxlineLayout::autoGainToggleBounds);

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
    outputValueLabel.setBounds(VoxlineLayout::outputValueBounds);

    presetDropdown.setBounds(VoxlineLayout::presetDropdownBounds);
    abButton.setBounds(VoxlineLayout::abButtonBounds);
    listenButton.setBounds(VoxlineLayout::listenUtilityBounds);
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

    // Labels — fonts sizes for output panel
    logoLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    subtitleLabel.setFont(juce::FontOptions(11.0f));
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

    // === Header ===
    // Bypass text colour (checkbox hidden by LookAndFeel)
    bypassButton.setColour(juce::ToggleButton::textColourId, theme.textPrimary);
    bypassButton.setColour(juce::ToggleButton::textColourId, theme.textPrimary);
    bypassButton.setColour(juce::ToggleButton::tickColourId, theme.accentRose);

    // === Bottom bar utility buttons ===
    const auto inactiveBg = dark ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfffaf7f2);
    abButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    abButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);

    // === Preset dropdown ===
    VoxlinePresetDropdownLNF::currentDropdownTheme = index;
    VoxlineAutoGainLNF::currentAutoGainTheme = index;
    presetDropdown.getProperties().set("themeIndex", index);
    presetDropdown.repaint();

    // === Toggles ===
    autoGainButton.setColour(juce::ToggleButton::textColourId, 
        dark ? juce::Colour(0xff9d99a8) : juce::Colour(0xff666666));
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

    draw(cachedBypassIcon.get(),  910, 74, 18, 18);
    draw(cachedListenIcon.get(),  964, 623, 20, 20);
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
}

void VoxlineAudioProcessorEditor::applyPreset(const juce::String& name)
{
    auto& apvts = audioProcessor.getAPVTS();

    // Map name to dropdown ID
    static const std::vector<juce::String> presetIds = {"Clean","Westwood","2Hollis","Yeat","Ken Carson","Bladee","Travis Scott","The Weeknd","Billie Eilish"};

    // Find preset index
    int idx = -1;
    for (int i = 0; i < (int)presetIds.size(); ++i)
    {
        if (presetIds[i] == name) { idx = i; break; }
    }
    if (idx < 0) return;

    // Preset table: { inGain, autoGain, polish, body, clarity, air, smooth, comp, drive, outGain }
    struct PresetDef { float in; bool ag; float pol, bd, cl, ar, sm, cp, dr, out; };
    static const PresetDef presets[] = {
        {  0.0f, true, 35, 45, 45, 35, 30, 25,  5,  0.0f }, // Clean
        { -1.0f, true, 76, 66, 74, 52, 30, 68, 32, -1.5f }, // Westwood
        { -1.5f, true, 84, 38, 82, 86, 42, 72, 28, -2.0f }, // 2Hollis
        { -1.5f, true, 80, 68, 64, 42, 34, 76, 42, -2.0f }, // Yeat
        { -2.0f, true, 82, 46, 84, 68, 30, 78, 44, -2.5f }, // Ken Carson
        { -1.0f, true, 70, 34, 66, 78, 62, 52, 12, -1.5f }, // Bladee
        { -1.0f, true, 74, 62, 62, 56, 48, 64, 22, -1.5f }, // Travis Scott
        { -1.0f, true, 78, 52, 68, 76, 60, 58, 10, -1.5f }, // The Weeknd
        {  0.0f, true, 58, 42, 48, 38, 72, 42,  4, -0.5f }, // Billie Eilish
    };
    auto& p = presets[idx];

    auto setParam = [&](const juce::String& id, float value) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(value);
    };

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

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
        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.drawText(button.getButtonText(), textArea, juce::Justification::centredLeft, false);
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
};

static VoxlineToggleLookAndFeel voxlineToggleLNF;

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
    configureHeaderButton(presetSelectorButton, "Preset: Warm");
    configureHeaderButton(settingsButton, "");
    configureHeaderButton(cleanModeButton, "Clean");

    configureTextLabel(inputTitleLabel, "INPUT", juce::Justification::centred);
    configureTextLabel(toneTitleLabel, "TONE CONTROLS", juce::Justification::centred);
    configureTextLabel(polishTitleLabel, "POLISH", juce::Justification::centred);
    configureTextLabel(outputTitleLabel, "OUTPUT", juce::Justification::centred);
    configureTextLabel(peakRmsLabel, "PEAK -3.4\nRMS -14.8", juce::Justification::centred);
    configureTextLabel(meterNamesLabel, "OUT   GR", juce::Justification::centred);

    configurePresetButton(cleanPresetButton, "Clean");
    configurePresetButton(warmPresetButton, "Warm", true);
    configurePresetButton(brightPresetButton, "Bright");
    configurePresetButton(rapPresetButton, "Rap");
    configurePresetButton(abButton, "A/B");

    cleanPresetButton.addListener(this);
    warmPresetButton.addListener(this);
    brightPresetButton.addListener(this);
    rapPresetButton.addListener(this);
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
    configureButton(listenButton, "  Listen");
    listenButton.setLookAndFeel(&voxlineToggleLNF);

    addAndMakeVisible(outputMeter);
    addAndMakeVisible(gainReductionMeter);
    outputMeter.setLevel(outputMeterValue);
    gainReductionMeter.setLevel(gainReductionMeterValue);

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

    setSize(VoxlineLayout::editorWidth, VoxlineLayout::editorHeight);
    setResizable(false, false);

    addKeyListener(this);
    setWantsKeyboardFocus(true);

    loadIconDrawables(false);
    applyTheme(VoxlineTheme::light, 0);
}

VoxlineAudioProcessorEditor::~VoxlineAudioProcessorEditor()
{
    audioProcessor.getAPVTS().removeParameterListener(VoxlineParameterIDs::polish, this);
    removeKeyListener(this);
    bypassButton.setLookAndFeel(nullptr);
    listenButton.setLookAndFeel(nullptr);
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
    presetSelectorButton.setBounds(VoxlineLayout::presetSelectorBounds);
    bypassButton.setBounds(VoxlineLayout::bypassButtonBounds);
    settingsButton.setBounds(VoxlineLayout::settingsButtonBounds);

    inputTitleLabel.setBounds(VoxlineLayout::inputTitleBounds);
    inputGainSlider.setBounds(VoxlineLayout::inputGainSliderBounds);
    autoGainButton.setBounds(VoxlineLayout::autoGainToggleBounds);
    cleanModeButton.setBounds(VoxlineLayout::cleanModeBounds);

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

    cleanPresetButton.setBounds(VoxlineLayout::cleanPresetBounds);
    warmPresetButton.setBounds(VoxlineLayout::warmPresetBounds);
    brightPresetButton.setBounds(VoxlineLayout::brightPresetBounds);
    rapPresetButton.setBounds(VoxlineLayout::rapPresetBounds);
    abButton.setBounds(VoxlineLayout::abButtonBounds);
    listenButton.setBounds(VoxlineLayout::listenUtilityBounds);
}

// ---------------------------------------------------------------------------
// Parameter listener
// ---------------------------------------------------------------------------
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
    // Preset selector — dark pill, always light text
    presetSelectorButton.setColour(juce::TextButton::buttonColourId, dark ? theme.panelBg : juce::Colour(0xff2f2c38));
    presetSelectorButton.setColour(juce::TextButton::textColourOffId, 
        dark ? theme.textPrimary : juce::Colour(0xffF5F0EA));

    // Settings — subtle secondary, transparent or light fill
    settingsButton.setColour(juce::TextButton::buttonColourId, 
        dark ? juce::Colour(0x00ffffff) : juce::Colour(0xffece5de));
    settingsButton.setColour(juce::TextButton::textColourOffId, theme.textSecondary);

    // Bypass text colour (checkbox hidden by LookAndFeel)
    bypassButton.setColour(juce::ToggleButton::textColourId, theme.textPrimary);

    // Clean button
    cleanModeButton.setColour(juce::TextButton::buttonColourId, theme.panelBg);
    cleanModeButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);

    // === Bottom preset bar ===
    const auto inactiveBg = dark ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfffaf7f2);
    const auto inactiveText = theme.textPrimary;

    cleanPresetButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    cleanPresetButton.setColour(juce::TextButton::textColourOffId, inactiveText);

    warmPresetButton.setColour(juce::TextButton::buttonColourId, theme.accentRose.withAlpha(dark ? 0.30f : 0.22f));
    warmPresetButton.setColour(juce::TextButton::textColourOffId, theme.accentRose);

    brightPresetButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    brightPresetButton.setColour(juce::TextButton::textColourOffId, inactiveText);

    rapPresetButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    rapPresetButton.setColour(juce::TextButton::textColourOffId, inactiveText);

    abButton.setColour(juce::TextButton::buttonColourId, inactiveBg);
    abButton.setColour(juce::TextButton::textColourOffId, inactiveText);

    // === Toggles ===
    autoGainButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);
    listenButton.setColour(juce::ToggleButton::textColourId, theme.textSecondary);

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
    draw(cachedSettingsIcon.get(), 1013, 71, 24, 24);
    draw(cachedListenIcon.get(),  956, 623, 20, 20);
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

    for (int i = 0; i < numDots; ++i)
    {
        const float cx = bounds.getX() + (float)i * spacing;
        if (i < 4) { g.setColour(t.accentLavender); g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f); }
        else       { g.setColour(t.textMuted.withAlpha(0.35f)); g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.0f); }
    }
}

// ---------------------------------------------------------------------------
// Preset system
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &cleanPresetButton)       applyPreset("Clean");
    else if (button == &warmPresetButton)   applyPreset("Warm");
    else if (button == &brightPresetButton) applyPreset("Bright");
    else if (button == &rapPresetButton)    applyPreset("Rap");
    else if (button == &abButton)           toggleAb();
}

void VoxlineAudioProcessorEditor::applyPreset(const juce::String& name)
{
    auto& apvts = audioProcessor.getAPVTS();

    struct Preset { juce::String id; float value; };
    std::vector<Preset> values;

    if (name == "Clean")
        values = {
            {"polish", 0.40f}, {"body", 0.40f}, {"clarity", 0.45f},
            {"air", 0.35f}, {"smooth", 0.20f}, {"comp", 0.20f}, {"drive", 0.05f}
        };
    else if (name == "Warm")
        values = {
            {"polish", 0.65f}, {"body", 0.70f}, {"clarity", 0.50f},
            {"air", 0.40f}, {"smooth", 0.30f}, {"comp", 0.40f}, {"drive", 0.35f}
        };
    else if (name == "Bright")
        values = {
            {"polish", 0.60f}, {"body", 0.45f}, {"clarity", 0.75f},
            {"air", 0.70f}, {"smooth", 0.50f}, {"comp", 0.35f}, {"drive", 0.15f}
        };
    else if (name == "Rap")
        values = {
            {"polish", 0.75f}, {"body", 0.55f}, {"clarity", 0.65f},
            {"air", 0.45f}, {"smooth", 0.25f}, {"comp", 0.70f}, {"drive", 0.55f}
        };
    else
        return;

    for (auto& p : values)
    {
        if (auto* param = apvts.getParameter(p.id))
            param->setValueNotifyingHost(p.value);
    }

    presetSelectorButton.setButtonText("Preset: " + name);

    // Highlight active preset button by swapping bg/text
    auto& t = VoxlineTheme::get(currentThemeIndex);
    const auto activeBg = t.accentRose.withAlpha(t.editorBg.getBrightness() < 0.3f ? 0.30f : 0.22f);
    const auto activeText = t.accentRose;
    const auto inactiveBg = (t.editorBg.getBrightness() < 0.3f) ? juce::Colour(0xff1e1b2a) : juce::Colour(0xfffaf7f2);
    const auto inactiveText = t.textPrimary;

    auto styleBtn = [&](juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId, active ? activeBg : inactiveBg);
        btn.setColour(juce::TextButton::textColourOffId, active ? activeText : inactiveText);
    };

    styleBtn(cleanPresetButton, name == "Clean");
    styleBtn(warmPresetButton, name == "Warm");
    styleBtn(brightPresetButton, name == "Bright");
    styleBtn(rapPresetButton, name == "Rap");

    DBG("VOXLINE preset: " << name);
}

void VoxlineAudioProcessorEditor::captureAbSlot(bool slotA)
{
    auto& target = slotA ? abSlotA : abSlotB;
    target.clear();
    auto& apvts = audioProcessor.getAPVTS();
    for (auto& id : {"polish","body","clarity","air","smooth","comp","drive","inputGain","outputGain"})
    {
        if (auto* p = apvts.getParameter(id))
            target[id] = p->getValue();
    }
}

void VoxlineAudioProcessorEditor::restoreAbSlot(bool slotA)
{
    auto& source = slotA ? abSlotA : abSlotB;
    if (source.empty()) return;
    auto& apvts = audioProcessor.getAPVTS();
    for (auto& [id, value] : source)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(value);
    }
}

void VoxlineAudioProcessorEditor::toggleAb()
{
    if (!abActive)
    {
        // First press: save current state to A, we're now active
        captureAbSlot(true);
        abActive = true;
        abButton.setButtonText("B");
    }
    else
    {
        // Toggle between A and B
        if (abButton.getButtonText() == "B")
        {
            captureAbSlot(false);  // save current (which is A) to B before switching
            restoreAbSlot(false);  // restore B
            abButton.setButtonText("A");
        }
        else
        {
            captureAbSlot(true);   // save current (which is B) to A before switching
            restoreAbSlot(true);   // restore A
            abButton.setButtonText("B");
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void VoxlineAudioProcessorEditor::configureKnob(VoxlineCustomKnob& knob)           { addAndMakeVisible(knob); }
void VoxlineAudioProcessorEditor::configureButton(juce::ToggleButton& b, const juce::String& t) { b.setButtonText(t); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configureHeaderButton(juce::TextButton& b, const juce::String& t) { b.setButtonText(t); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configurePresetButton(juce::TextButton& b, const juce::String& t, bool) { b.setButtonText(t); b.setEnabled(true); addAndMakeVisible(b); }
void VoxlineAudioProcessorEditor::configureTextLabel(juce::Label& l, const juce::String& t, juce::Justification j) { l.setText(t, juce::dontSendNotification); l.setJustificationType(j); addAndMakeVisible(l); }

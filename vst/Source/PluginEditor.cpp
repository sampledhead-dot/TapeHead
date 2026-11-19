#include "PluginEditor.h"
#include "Presets.h"

static constexpr float kPi = juce::MathConstants<float>::pi;

//==============================================================================
TapeLookAndFeel::TapeLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::silk);
    setColour (juce::PopupMenu::headerTextColourId, col::orangeL);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::orange);
    setColour (juce::PopupMenu::highlightedTextColourId, col::ink);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::silk);
    setColour (juce::AlertWindow::outlineColourId, col::orange);
    setColour (juce::TextEditor::backgroundColourId, col::cream);
    setColour (juce::TextEditor::textColourId, col::ink);
    setColour (juce::TextEditor::outlineColourId, col::ink);
    setColour (juce::TextButton::buttonColourId, col::metal1);
    setColour (juce::TextButton::textColourOffId, col::ink);
    setColour (juce::TextButton::textColourOnId, col::ink);
}

// Mando de consola de los 80: escala serigrafiada, faldón negro estriado, capuchón de color y raya crema
void TapeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const bool big = s.getProperties()["big"];
    const auto cap = juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["cap"]);
    const float S = (float) juce::jmin (w, h), cx = x + w * 0.5f, cy = y + h * 0.5f, u = S / 100.0f;
    const float R = 46.0f * u;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f, sn = std::sin (a), cs = -std::cos (a);
        g.setColour (col::silk);
        g.drawLine (cx + R * sn, cy + R * cs, cx + (R - 5 * u) * sn, cy + (R - 5 * u) * cs, i % 5 == 0 ? 2.0f * u : 1.2f * u);
        if (big && i % 2 == 0)
        {
            g.setFont (juce::FontOptions (7.5f * u, juce::Font::bold));
            g.drawText (juce::String (i), juce::Rectangle<float> (14 * u, 10 * u).withCentre ({ cx + (R + 7 * u) * sn, cy + (R + 7 * u) * cs }), juce::Justification::centred);
        }
    }
    const float rs = 35.0f * u, rc = 24.0f * u, ang = a0 + pos * (a1 - a0);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (cx - rs, cy - rs + 2.5f * u, rs * 2, rs * 2);
    g.setColour (col::panel2); g.fillEllipse (cx - rs, cy - rs, rs * 2, rs * 2);
    g.setColour (juce::Colour (0xff45423c)); g.drawEllipse (cx - rs, cy - rs, rs * 2, rs * 2, 1.5f * u);
    g.setColour (juce::Colours::black);
    for (int i = 0; i < 24; ++i)
    {
        const float a = ang + (float) i * kPi / 12.0f;
        g.drawLine (cx + std::sin (a) * 31 * u, cy - std::cos (a) * 31 * u, cx + std::sin (a) * 35 * u, cy - std::cos (a) * 35 * u, 1.6f * u);
    }
    g.setColour (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2);
    juce::ColourGradient hl (juce::Colours::white.withAlpha (0.45f), cx - rc * 0.3f, cy - rc * 0.4f, juce::Colours::black.withAlpha (0.3f), cx + rc, cy + rc, true);
    hl.addColour (0.55, juce::Colours::white.withAlpha (0.0f));
    g.setGradientFill (hl); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2);
    juce::Path ptr; ptr.addRoundedRectangle (-1.6f * u, -36.0f * u, 3.2f * u, 21.0f * u, 1.4f * u);
    g.setColour (col::cream); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
}

// Teclas de transporte: plástico crema que se hunde al pulsar; la encendida queda naranja
void TapeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState(), pressed = on || down;
    const float depth = pressed ? 2.0f : 6.0f;
    auto face = r.withTrimmedBottom (6.0f).translated (0, pressed ? 4.0f : 0.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (0, 2), 6.0f);
    g.setColour (juce::Colour (0xff5f5c56)); g.fillRoundedRectangle (face.withHeight (face.getHeight() + depth), 6.0f);
    juce::ColourGradient grad (on ? juce::Colour (0xffffb36b) : col::cream, 0, face.getY(), on ? col::orangeD : col::metal2, 0, face.getBottom(), false);
    grad.addColour (0.7, on ? col::orange : juce::Colour (0xffc9c5bc));
    g.setGradientFill (grad); g.fillRoundedRectangle (face, 5.0f);
    if (over && ! on) { g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRoundedRectangle (face, 5.0f); }
    if (! b.isEnabled()) { g.setColour (col::panel.withAlpha (0.5f)); g.fillRoundedRectangle (face, 5.0f); }
}

void TapeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const bool pressed = b.getToggleState() || down;
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (6.0f).translated (0, pressed ? 4.0f : 0.0f);
    const auto icon = b.getProperties()["icon"].toString(), sub = b.getProperties()["sub"].toString();
    auto top = r.withTrimmedBottom (r.getHeight() * 0.42f), bot = r.withTrimmedTop (r.getHeight() * 0.58f);
    g.setColour (col::ink.withAlpha (b.isEnabled() ? 1.0f : 0.4f));
    const float cx = top.getCentreX(), cy = top.getCentreY() + 2, s = 7.0f;
    juce::Path p;
    if (icon == "rew" || icon == "ff")
    {
        const float d = icon == "ff" ? 1.0f : -1.0f;
        for (int k = 0; k < 2; ++k) { const float ox = cx + d * (k == 0 ? -s : 0.0f) + (d < 0 ? s * 0.5f : -s * 0.5f); p.addTriangle (ox - d * s * 0.5f, cy - s, ox - d * s * 0.5f, cy + s, ox + d * s * 0.9f, cy); }
        g.fillPath (p);
    }
    else if (icon == "eject")
    {
        p.addTriangle (cx - s, cy + 1, cx + s, cy + 1, cx, cy - s); g.fillPath (p); g.fillRect (cx - s, cy + 4, s * 2, 3.0f);
    }
    else if (icon == "rec")
    {
        g.setColour (col::red); g.fillEllipse (cx - 6, cy - 6, 12, 12);
    }
    else
    {
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold)); g.drawText (b.getButtonText(), top, juce::Justification::centredBottom);
    }
    g.setColour (col::ink.withAlpha (b.isEnabled() ? 0.7f : 0.3f));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold).withKerningFactor (0.16f));
    g.drawText (sub, bot, juce::Justification::centredTop);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l, juce::Colour cap, bool b)
    : label (l), paramId (id), state (s), big (b)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kPi * 1.25f, kPi * 2.75f, true);
    slider.getProperties().set ("cap", (juce::int64) cap.getARGB());
    slider.getProperties().set ("big", big);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}
void Knob::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (36)); }
void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().removeFromBottom (36);
    g.setColour (col::silk);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold).withKerningFactor (0.2f));
    g.drawText (label, r.removeFromTop (17), juce::Justification::centred);
    g.setColour (col::amber);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::plain));
    if (auto* prm = state.getParameter (paramId)) g.drawText (prm->getCurrentValueAsText(), r, juce::Justification::centredTop);
}

//==============================================================================
TapeHeadEditor::TapeHeadEditor (TapeHeadProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;
    kDrive = std::make_unique<Knob> (s, ids::drive, "DRIVE", col::red, true);
    kBass  = std::make_unique<Knob> (s, ids::bass, "GRAVES", col::blue);
    kTreble = std::make_unique<Knob> (s, ids::treble, "AGUDOS", col::orange);
    kMix   = std::make_unique<Knob> (s, ids::mix, "MIX", col::yellow);
    kIn    = std::make_unique<Knob> (s, ids::inputDb, "INPUT", col::grey);
    kOut   = std::make_unique<Knob> (s, ids::outputDb, "OUTPUT", col::grey);
    for (auto* k : { kDrive.get(), kBass.get(), kTreble.get(), kMix.get(), kIn.get(), kOut.get() }) addAndMakeVisible (k);

    auto setupKey = [this] (juce::TextButton& b, const char* icon, const char* sub, const char* tip)
    {
        b.getProperties().set ("icon", icon); b.getProperties().set ("sub", u8 (sub)); b.setTooltip (u8 (tip));
        addAndMakeVisible (b);
    };
    setupKey (rewB, "rew", "PRESET", "Preset anterior");
    setupKey (ffB, "ff", "PRESET", "Preset siguiente");
    autoB.setButtonText ("AUTO"); setupKey (autoB, "", "GAIN", "Auto-Gain: mantiene el nivel aunque subas el Drive");
    setupKey (recB, "rec", "GUARDAR", "Guardar como preset de usuario");
    setupKey (ejectB, "eject", "BORRAR", "Borrar el preset de usuario actual");
    autoB.setClickingTogglesState (true);
    autoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, ids::autoGain, autoB);
    rewB.onClick = [this] { stepPreset (-1); };
    ffB.onClick = [this] { stepPreset (1); };
    recB.onClick = [this] { askSave(); };
    ejectB.onClick = [this] { askDelete(); };

    addChildComponent (update);
    setSize (860, 664);
    lastTime = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

TapeHeadEditor::~TapeHeadEditor() { setLookAndFeel (nullptr); }

void TapeHeadEditor::resized()
{
    kDrive->setBounds (24, 408, 170, 168);
    const int kw = 110, x0 = 212, gap = 15;
    Knob* small[] = { kBass.get(), kTreble.get(), kMix.get(), kIn.get(), kOut.get() };
    for (int i = 0; i < 5; ++i) small[i]->setBounds (x0 + i * (kw + gap), 446, kw, 130);
    juce::TextButton* keys[] = { &rewB, &ffB, &autoB, &recB, &ejectB };
    const int kx = 20, kwid = (820 - 4 * 8) / 5;
    for (int i = 0; i < 5; ++i) keys[i]->setBounds (kx + i * (kwid + 8), 590, kwid, 50);
    update.setBounds (getWidth() - 496, getHeight() - 36, 480, 30);
}

//==============================================================================
juce::StringArray TapeHeadEditor::presetList() const
{
    juce::StringArray l;
    for (auto& f : factoryPresets()) l.add (u8 (f.name));
    l.addArray (proc.getUserPresetNames());
    return l;
}
void TapeHeadEditor::loadByName (const juce::String& name)
{
    const auto& f = factoryPresets();
    for (int i = 0; i < (int) f.size(); ++i) if (u8 (f[(size_t) i].name) == name) { proc.loadFactory (i); repaint(); return; }
    proc.loadUserPreset (name); repaint();
}
void TapeHeadEditor::stepPreset (int dir)
{
    const auto l = presetList();
    const int i = l.indexOf (proc.currentPresetName), n = l.size();
    loadByName (l[((i < 0 ? (dir > 0 ? -1 : 0) : i) + dir + n) % n]);
}
void TapeHeadEditor::showPresetMenu()
{
    juce::PopupMenu m;
    const auto& f = factoryPresets();
    juce::String cat;
    for (int i = 0; i < (int) f.size(); ++i)
    {
        if (u8 (f[(size_t) i].category) != cat) { cat = u8 (f[(size_t) i].category); m.addSectionHeader (cat.toUpperCase()); }
        m.addItem (i + 1, u8 (f[(size_t) i].name), true, proc.currentPresetName == u8 (f[(size_t) i].name));
    }
    const auto user = proc.getUserPresetNames();
    if (! user.isEmpty())
    {
        m.addSectionHeader ("Usuario");
        for (int i = 0; i < user.size(); ++i) m.addItem (1000 + i, user[i], true, proc.currentPresetName == user[i]);
    }
    m.addSeparator();
    m.addItem (5000, u8 ("Guardar como…"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (counterArea)),
                     [this, user] (int r)
                     {
                         if (r <= 0) return;
                         if (r == 5000) { askSave(); return; }
                         if (r < 1000) proc.loadFactory (r - 1); else proc.loadUserPreset (user[r - 1000]);
                         repaint();
                     });
}
void TapeHeadEditor::askSave()
{
    auto* w = new juce::AlertWindow ("Guardar preset", "Nombre del preset:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.currentPresetName == "Default" ? juce::String() : proc.currentPresetName);
    w->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
    {
        const auto name = w->getTextEditorContents ("name").trim();
        if (r != 1 || name.isEmpty()) return;
        if (name == "Default") { juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "TapeHead", u8 ("\"Default\" está reservado.")); return; }
        auto doSave = [this, name] { proc.saveUserPreset (name); repaint(); };
        if (proc.getUserPresetNames().contains (name))
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("TapeHead")
                                          .withMessage ("Ya existe \"" + name + u8 ("\". ¿Sobrescribir?")).withButton (u8 ("Sí"), 1).withButton ("No", 0).withAssociatedComponent (this),
                                          [doSave] (int ok) { if (ok == 1) doSave(); });
        else doSave();
    }), true);
}
void TapeHeadEditor::askDelete()
{
    const auto name = proc.currentPresetName;
    if (! proc.getUserPresetNames().contains (name)) return;
    juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("TapeHead")
                                  .withMessage (u8 ("¿Borrar el preset \"") + name + "\"?").withButton ("Borrar", 1).withButton ("Cancelar", 0).withAssociatedComponent (this),
                                  [this, name] (int ok) { if (ok == 1) { proc.deleteUserPreset (name); proc.loadFactory (0); repaint(); } });
}
void TapeHeadEditor::mouseUp (const juce::MouseEvent& e)
{
    if (deckArea.contains (e.getPosition()) || counterArea.contains (e.getPosition())) showPresetMenu();
}

//==============================================================================
void TapeHeadEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = (float) juce::jlimit (0.0, 0.1, (now - lastTime) / 1000.0);
    lastTime = now;
    // Vúmetro: 0 VU = −18 dBFS RMS, balística de ~300 ms
    const float target = juce::jlimit (-20.0f, 3.0f, proc.meterDb.load() + 18.0f);
    vu += (target - vu) * (1.0f - std::exp (-dt / 0.3f));
    const int act = proc.activity.load();
    if (act != lastActivity) { lastActivity = act; reelSpeed = 1.0f; } else reelSpeed = std::max (0.0f, reelSpeed - dt * 0.8f);
    reelAngle = std::fmod (reelAngle + dt * reelSpeed * 2.4f, kPi * 2);
    if (proc.peakHit.exchange (false)) peakUntil = now + 250;
    ejectB.setEnabled (proc.getUserPresetNames().contains (proc.currentPresetName));
    repaint (deckArea); repaint (vuArea); repaint (ledArea); repaint (counterArea);
}

//==============================================================================
void TapeHeadEditor::drawTop (juce::Graphics& g)
{
    const juce::Rectangle<float> strip (0, 0, (float) getWidth(), 92);
    {   // metal cepillado
        juce::ColourGradient mg (col::metal1, 0, 0, col::metal2, 0, strip.getBottom(), false);
        g.setGradientFill (mg); g.fillRect (strip);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (int x = 0; x < getWidth(); x += 3) g.drawVerticalLine (x, 0, strip.getBottom());
        g.setColour (juce::Colour (0xff0c0c0b)); g.fillRect (0.0f, strip.getBottom(), strip.getWidth(), 3.0f);
    }
    {   // gran círculo naranja (como en el logo de Sampled Head)
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (strip.toNearestInt());
        g.setColour (col::orange); g.fillEllipse (-70.0f, -200.0f, 300.0f, 300.0f);
    }
    {   // icono: casete sobre el círculo naranja
        const float cx = 50.0f, cy = 47.0f;
        g.setColour (col::orangeD); g.fillEllipse (cx - 30, cy - 28.5f, 60, 60);
        g.setColour (col::orange);  g.fillEllipse (cx - 30, cy - 30, 60, 60);
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (-0.1f, cx, cy));
        const float u = 60.0f / 64.0f, ox = cx - 32 * u, oy = cy - 32 * u;
        auto R = [&] (float x, float y, float w, float h) { return juce::Rectangle<float> (ox + x * u, oy + y * u, w * u, h * u); };
        g.setColour (col::ink); g.fillRoundedRectangle (R (5, 15, 54, 35), 3 * u);
        g.setColour (col::cream); g.fillRoundedRectangle (R (8, 18, 48, 20), 1.5f * u);
        g.setColour (col::orange); g.fillRect (R (8, 18, 48, 4));
        g.setColour (col::ink); g.fillRoundedRectangle (R (17, 25, 30, 10), 5 * u);
        g.setColour (col::cream); g.fillEllipse (R (19.4f, 26.4f, 7.2f, 7.2f)); g.fillEllipse (R (37.4f, 26.4f, 7.2f, 7.2f));
        juce::Path tr; tr.startNewSubPath (ox + 16 * u, oy + 50 * u); tr.lineTo (ox + 19 * u, oy + 42 * u); tr.lineTo (ox + 45 * u, oy + 42 * u); tr.lineTo (ox + 48 * u, oy + 50 * u); tr.closeSubPath();
        g.setColour (juce::Colour (0xff2b2622)); g.fillPath (tr);
    }
    {   // palabra TAPEHEAD: relleno negro, contorno crema, sombra y goteos (grafiti)
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (42.0f, juce::Font::bold | juce::Font::italic)), "TAPEHEAD", 0, 0);
        juce::Path p; ga.createPath (p);
        p.applyTransform (juce::AffineTransform::rotation (-0.05f).translated (94.0f, 62.0f));
        g.setColour (juce::Colour (0xff2b2622)); g.fillPath (p, juce::AffineTransform::translation (4.0f, 4.0f));
        g.setColour (col::cream); g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col::ink); g.fillPath (p);
        for (auto d : { std::pair<float, float> { 112.0f, 13.0f }, { 166.0f, 19.0f }, { 222.0f, 10.0f }, { 272.0f, 16.0f } })
        {
            const float yy = 62.0f - (d.first - 94.0f) * 0.05f;
            g.setColour (col::cream); g.fillRoundedRectangle (d.first - 3.0f, yy - 2.0f, 7.0f, d.second + 3.0f, 3.5f);
            g.setColour (col::ink);   g.fillRoundedRectangle (d.first - 1.5f, yy - 2.0f, 4.0f, d.second, 2.0f);
        }
    }
    // serigrafía del modelo
    g.setColour (col::ink);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold).withKerningFactor (0.3f));
    g.drawText ("MULTITRACK PREAMP", 420, 20, 420, 18, juce::Justification::centredRight);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold).withKerningFactor (0.22f));
    g.setColour (col::ink.withAlpha (0.75f));
    g.drawText (u8 ("CASSETTE CONSOLE · MODEL TH-1"), 420, 40, 420, 14, juce::Justification::centredRight);
    const juce::Colour bars[] = { col::red, col::yellow, col::blue, col::orange };
    for (int i = 0; i < 4; ++i) { g.setColour (bars[i]); g.fillRoundedRectangle (744.0f + i * 25.0f, 62.0f, 21.0f, 5.0f, 2.0f); }
}

void TapeHeadEditor::drawCassette (juce::Graphics& g, juce::Rectangle<float> area)
{
    // ventana ahumada
    g.setColour (juce::Colour (0xff0d0d0c)); g.fillRoundedRectangle (area, 10.0f);
    g.setColour (col::line); g.drawRoundedRectangle (area.reduced (1.0f), 10.0f, 2.0f);
    auto in = area.reduced (10.0f);
    const float sx = in.getWidth() / 340.0f, sy = in.getHeight() / 190.0f;
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::scale (sx, sy).translated (in.getX(), in.getY()));
    // carcasa
    g.setColour (juce::Colour (0xff2b2a27)); g.fillRoundedRectangle (6, 6, 328, 178, 10);
    g.setColour (juce::Colour (0xff45423c)); g.drawRoundedRectangle (6, 6, 328, 178, 10, 2);
    g.setColour (juce::Colour (0xff1a1918));
    for (auto pt : { juce::Point<float> (18, 18), { 322, 18 }, { 18, 172 }, { 322, 172 } }) g.fillEllipse (pt.x - 4, pt.y - 4, 8, 8);
    // etiqueta
    g.setColour (col::cream); g.fillRoundedRectangle (22, 16, 296, 118, 5);
    g.setColour (col::orange); g.fillRoundedRectangle (22, 16, 296, 16, 5); g.fillRect (22.0f, 26.0f, 296.0f, 6.0f);
    g.setColour (col::ink); g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.24f));
    g.drawText (u8 ("SAMPLED HEAD · C-90"), 32, 18, 220, 13, juce::Justification::centredLeft);
    g.drawText ("A", 280, 18, 28, 13, juce::Justification::centredRight);
    g.setColour (juce::Colour (0xffc9c2b6)); g.drawHorizontalLine (50, 32, 308); g.drawHorizontalLine (122, 32, 308);
    {   // nombre del preset escrito a mano
       #if JUCE_MAC
        const char* hand = "Marker Felt";
       #elif JUCE_WINDOWS
        const char* hand = "Segoe Print";
       #else
        const char* hand = "";
       #endif
        juce::Graphics::ScopedSaveState s2 (g);
        g.addTransform (juce::AffineTransform::rotation (-0.026f, 40, 46));
        g.setColour (col::pen);
        auto f = *hand ? juce::FontOptions (hand, 18.0f, juce::Font::plain) : juce::FontOptions (18.0f, juce::Font::italic);
        g.setFont (f);
        g.drawText (proc.currentPresetName == "Default" ? juce::String ("TapeHead") : proc.currentPresetName, 40, 30, 268, 22, juce::Justification::bottomLeft);
    }
    // ventana de la cinta y bobinas
    g.setColour (juce::Colour (0xff141312)); g.fillRoundedRectangle (78, 60, 184, 56, 28);
    g.setColour (juce::Colour (0xff3b2a20)); g.fillRect (146.0f, 70.0f, 48.0f, 36.0f);
    auto reel = [&] (float cx, float cy, float pack, float ang)
    {
        g.setColour (juce::Colour (0xff3b2a20)); g.fillEllipse (cx - pack, cy - pack, pack * 2, pack * 2);
        g.setColour (juce::Colour (0xff5a4232)); g.drawEllipse (cx - pack, cy - pack, pack * 2, pack * 2, 1.0f);
        g.setColour (juce::Colour (0xffeae6de)); g.fillEllipse (cx - 15, cy - 15, 30, 30);
        g.setColour (juce::Colour (0xff1a1918));
        for (int i = 0; i < 6; ++i)
        {
            const float a = ang + (float) i * kPi / 3.0f;
            g.fillEllipse (cx + std::cos (a) * 9.5f - 3.6f, cy + std::sin (a) * 9.5f - 3.6f, 7.2f, 7.2f);
            juce::Path t; t.addRectangle (-1.4f, -5.6f, 2.8f, 3.0f);
            g.fillPath (t, juce::AffineTransform::rotation (a + kPi / 6.0f).translated (cx, cy));
        }
        g.fillEllipse (cx - 3, cy - 3, 6, 6);
    };
    reel (118, 88, 26, reelAngle);
    reel (222, 88, 19, reelAngle * 1.35f);
    // parte de abajo (cabezales)
    juce::Path tr; tr.startNewSubPath (70, 184); tr.lineTo (86, 146); tr.lineTo (254, 146); tr.lineTo (270, 184); tr.closeSubPath();
    g.setColour (juce::Colour (0xff232220)); g.fillPath (tr);
    g.setColour (juce::Colour (0xff0a0a09));
    g.fillEllipse (112, 160, 12, 12); g.fillEllipse (216, 160, 12, 12); g.fillRoundedRectangle (158, 158, 24, 12, 2);
    // brillo del cristal
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.10f), 0, 0, juce::Colours::transparentWhite, 120, 120, false);
    g.setGradientFill (gl); g.fillRoundedRectangle (0, 0, 340, 190, 6);
}

static float vuAngle (float db) { float t = (juce::jlimit (-20.0f, 3.0f, db) + 20.0f) / 23.0f; t = std::pow (t, 1.7f); return (-48.0f + 96.0f * t) * kPi / 180.0f; }

void TapeHeadEditor::drawVU (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (juce::Colour (0xff0d0d0c)); g.fillRoundedRectangle (area, 10.0f);
    g.setColour (col::line); g.drawRoundedRectangle (area.reduced (1.0f), 10.0f, 2.0f);
    auto in = area.reduced (10.0f);
    const float sx = in.getWidth() / 240.0f, sy = in.getHeight() / 150.0f;
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (in.toNearestInt());
    g.addTransform (juce::AffineTransform::scale (sx, sy).translated (in.getX(), in.getY()));
    juce::ColourGradient amb (juce::Colour (0xffffe7a8), 120, 135, juce::Colour (0xffe9b85a), 120, -40, true);
    g.setGradientFill (amb); g.fillRect (0.0f, 0.0f, 240.0f, 150.0f);
    const float cx = 120, cy = 150, r = 104;
    auto pt = [&] (float db, float rr) { const float a = vuAngle (db); return juce::Point<float> (cx + rr * std::sin (a), cy - rr * std::cos (a)); };
    juce::Path arc;  arc.addCentredArc (cx, cy, r, r, 0, vuAngle (-20), vuAngle (0), true);
    juce::Path red;  red.addCentredArc (cx, cy, r, r, 0, vuAngle (0), vuAngle (3), true);
    g.setColour (juce::Colour (0xff1a1a1a)); g.strokePath (arc, juce::PathStrokeType (2.0f));
    g.setColour (col::red); g.strokePath (red, juce::PathStrokeType (6.0f));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    for (int d : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
    {
        const auto c = d > 0 ? col::red : juce::Colour (0xff1a1a1a);
        const auto p1 = pt ((float) d, r), p2 = pt ((float) d, r + (d % 5 == 0 || d > -4 ? 9.0f : 5.0f));
        g.setColour (c); g.drawLine ({ p1, p2 }, 1.6f);
        if (d == -20 || d == -10 || d == -5 || d == -3 || d == 0 || d == 3)
            g.drawText (d > 0 ? "+" + juce::String (d) : juce::String (d), juce::Rectangle<float> (24, 12).withCentre (pt ((float) d, r + 19)), juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff1a1a1a));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold)); g.drawText ("VU", 90, 78, 60, 22, juce::Justification::centred);
    g.setFont (juce::FontOptions (7.0f, juce::Font::bold).withKerningFactor (0.3f));
    g.drawText (u8 ("TAPEHEAD · OUTPUT"), 40, 104, 160, 10, juce::Justification::centred);
    const auto tip = pt (vu, 112);
    g.setColour (col::ink); g.drawLine (cx, cy, tip.x, tip.y, 1.8f);
    g.setColour (juce::Colour (0xff2b2a27)); g.fillRect (0.0f, 128.0f, 240.0f, 22.0f);
    g.setColour (col::ink); g.fillEllipse (cx - 14, cy - 14, 28, 28);
}

void TapeHeadEditor::drawCounter (juce::Graphics& g)
{
    const auto l = presetList();
    const int idx = l.indexOf (proc.currentPresetName);
    const auto num = juce::String (idx < 0 ? 0 : idx).paddedLeft ('0', 3);
    g.setColour (col::silk.withAlpha (0.85f));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold).withKerningFactor (0.24f));
    g.drawText ("PRESET", 20, 368, 70, 22, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff0b0b0a)); g.fillRoundedRectangle (92, 366, 58, 27, 4);
    for (int i = 0; i < 3; ++i)
    {
        juce::Rectangle<float> d (95.0f + i * 18.0f, 369.0f, 16.0f, 21.0f);
        juce::ColourGradient wg (juce::Colour (0xfffafafa), 0, d.getY(), juce::Colour (0xffd8d4cc), 0, d.getCentreY(), false);
        g.setGradientFill (wg); g.fillRoundedRectangle (d, 2);
        g.setColour (col::ink); g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 15.0f, juce::Font::bold));
        g.drawText (num.substring (i, i + 1), d, juce::Justification::centred);
    }
    g.setColour (col::silk.withAlpha (0.7f));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold).withKerningFactor (0.12f));
    juce::String cat;
    for (auto& fp : factoryPresets()) if (u8 (fp.name) == proc.currentPresetName && proc.currentPresetName != "Default") cat = u8 (fp.category).toUpperCase() + u8 ("  ·  ");
    g.drawText (cat + proc.currentPresetName.toUpperCase() + u8 ("   ·   toca el casete para elegir preset"), 162, 368, 640, 22, juce::Justification::centredLeft);
}

void TapeHeadEditor::paint (juce::Graphics& g)
{
    // panel negro con rayado fino
    juce::ColourGradient bg (juce::Colour (0xff262522), 0, 92, juce::Colour (0xff1a1918), 0, (float) getHeight(), false);
    g.setGradientFill (bg); g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.018f));
    for (int y = 92; y < getHeight(); y += 3) g.drawHorizontalLine (y, 0, (float) getWidth());
    drawTop (g);
    drawCassette (g, deckArea.toFloat());
    drawVU (g, vuArea.toFloat());
    // pilotos
    const bool pk = juce::Time::getMillisecondCounterHiRes() < peakUntil, ag = proc.apvts.getRawParameterValue (ids::autoGain)->load() > 0.5f;
    auto led = [&] (float x, bool on, juce::Colour c, const char* txt)
    {
        if (on) { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (x - 4, 329, 18, 18); }
        g.setColour (on ? c : c.darker (2.2f)); g.fillEllipse (x, 333, 10, 10);
        g.setColour (col::silk.withAlpha (0.85f)); g.setFont (juce::FontOptions (10.5f, juce::Font::bold).withKerningFactor (0.24f));
        g.drawText (u8 (txt), (int) x + 16, 329, 120, 18, juce::Justification::centredLeft);
    };
    led (494, pk, juce::Colour (0xffff3b22), "PEAK");
    led (584, ag, juce::Colour (0xff62e24a), "AUTO-GAIN");
    drawCounter (g);
    g.setColour (col::line);
    g.fillRect (20, 402, 820, 1); g.fillRect (20, 580, 820, 1);
    g.setColour (col::silk.withAlpha (0.6f));
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::plain));
    g.drawText (u8 ("Entrada → TRIM → fader → bus → salida · latencia 32 muestras"), 20, getHeight() - 18, 520, 14, juce::Justification::centredLeft);
    if (! update.isVisible())
        g.drawText (u8 ("TapeHead · VST v") + TapeHeadProcessor::kVersion + u8 (" · Sampled Head"), getWidth() - 340, getHeight() - 18, 320, 14, juce::Justification::centredRight);
}

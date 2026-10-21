#include "CyberUI.h"

//==============================================================================
void Cyber::drawBevelPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour neon, float cut)
{
    auto p = chamfer (r, cut);

    juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 7, { 0, 3 }).drawForPath (g, p);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d1334), 0.f, r.getY(),
                                             juce::Colour (0xff0b0615), 0.f, r.getBottom(), false));
    g.fillPath (p);

    // luz en el borde superior y sombra interior inferior (relieve)
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawLine (r.getX() + cut, r.getY() + 1.5f, r.getRight() - 2.f, r.getY() + 1.5f, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (r.getX() + 2.f, r.getBottom() - 1.5f, r.getRight() - cut, r.getBottom() - 1.5f, 1.5f);

    g.setColour (neon.withAlpha (0.14f));
    g.strokePath (p, juce::PathStrokeType (5.f));
    g.setColour (neon.withAlpha (0.70f));
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

//==============================================================================
void CyberButton::timerCallback()
{
    const float tp = isDown() ? 1.f : 0.f, th = isOver() ? 1.f : 0.f;
    press += (tp - press) * 0.45f;
    hover += (th - hover) * 0.30f;
    if (std::abs (tp - press) < 0.01f) press = tp;
    if (std::abs (th - hover) < 0.01f) hover = th;
    repaint();
    if (press == tp && hover == th)
        stopTimer();
}

void CyberButton::paintButton (juce::Graphics& g, bool, bool)
{
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    const bool on = getToggleState();
    const bool en = isEnabled();

    const float depth = 2.5f * (1.f - press);                         // altura del relieve
    auto face = b.withTrimmedBottom (depth).translated (0.f, press * 2.f);   // al pulsar se hunde

    if (depth > 0.2f)                                                 // zócalo oscuro bajo el botón
    {
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillPath (Cyber::chamfer (b.withTrimmedTop (depth), 6.f));
    }

    auto path = Cyber::chamfer (face, 6.f);
    const float lum = 1.f + 0.20f * hover - 0.35f * press;

    juce::Colour top, bottom;
    if (on)
    {
        top    = neon.withMultipliedBrightness (0.65f * lum);
        bottom = neon.withMultipliedBrightness (0.28f * lum);
    }
    else
    {
        top    = juce::Colour (0xff30254c).withMultipliedBrightness (lum);
        bottom = juce::Colour (0xff130c22).withMultipliedBrightness (lum);
    }
    g.setGradientFill (juce::ColourGradient (top, 0.f, face.getY(), bottom, 0.f, face.getBottom(), false));
    g.fillPath (path);

    g.setColour (juce::Colours::white.withAlpha (0.22f * (1.f - press)));   // luz del borde superior
    g.drawLine (face.getX() + 8.f, face.getY() + 1.2f, face.getRight() - 3.f, face.getY() + 1.2f, 1.f);

    if (on)
    {
        g.setColour (neon.withAlpha (0.22f));
        g.strokePath (path, juce::PathStrokeType (5.f));
    }
    g.setColour (on ? neon : neon.withAlpha (0.40f + 0.40f * hover));
    g.strokePath (path, juce::PathStrokeType (1.3f));

    if (hasArrow)
    {
        juce::Path tri;
        tri.addTriangle (-0.55f, -0.8f, -0.55f, 0.8f, 0.95f, 0.f);
        const float s = juce::jmin (face.getWidth(), face.getHeight()) * 0.30f;
        tri.applyTransform (juce::AffineTransform::rotation (arrowDir * juce::MathConstants<float>::twoPi)
                                .scaled (s, s).translated (face.getCentreX(), face.getCentreY()));
        g.setColour (on ? juce::Colours::white : neon);
        g.fillPath (tri);
    }
    else
    {
        g.setColour (en ? juce::Colours::white.withAlpha (0.93f) : juce::Colours::grey);
        g.setFont (Cyber::font (small ? 11.5f : 13.5f, true));
        g.drawText (getButtonText(), face.toNearestInt().reduced (4, 0), juce::Justification::centred);
    }
}

//==============================================================================
void Region::setFullSize (int w, int h)
{
    if (cache.isValid() && cache.getWidth() == w && cache.getHeight() == h && getWidth() == w && getHeight() == h)
        return;

    setSize (w, h);

    if (framed && w > 0 && h > 0)
    {
        cache = juce::Image (juce::Image::ARGB, w, h, true);
        juce::Graphics g (cache);
        Cyber::drawBevelPanel (g, juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (5.f), neon, 12.f);
    }
}

void Region::paint (juce::Graphics& g)
{
    if (cache.isValid())
        g.drawImageAt (cache, 0, 0);
    if (paintOver)
        paintOver (g);
}

//==============================================================================
CyberLookAndFeel::CyberLookAndFeel()
{
    setColourScheme (juce::LookAndFeel_V4::getMidnightColourScheme());
    setDefaultSansSerifTypefaceName (Cyber::fontName());

    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::backgroundColourId, Cyber::panel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Cyber::magenta.withAlpha (0.35f));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colours::white);
}

void CyberLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float a0, float a1, juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float aTo = a0 + pos * (a1 - a0);
    const bool en = s.isEnabled();

    auto neon = s.findColour (juce::Slider::rotarySliderFillColourId);
    if (! en) neon = neon.withAlpha (0.35f);

    // base metálica con sombra
    const auto outer = juce::Rectangle<float> (radius * 2.f, radius * 2.f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (outer.translated (0.f, 2.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff463a66), outer.getX(), outer.getY(),
                                             juce::Colour (0xff0d0817), outer.getRight(), outer.getBottom(), false));
    g.fillEllipse (outer);

    // arco de valor
    const float arcR = radius - 4.f;
    juce::Path track, val;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, a0, a1, true);
    val.addCentredArc   (c.x, c.y, arcR, arcR, 0.f, a0, aTo, true);
    g.setColour (juce::Colour (0xff0a0612));
    g.strokePath (track, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (neon.withAlpha (0.22f));
    g.strokePath (val, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (neon);
    g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // cuerpo del botón con relieve
    const auto body = outer.reduced (radius * 0.36f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (body.translated (0.f, 1.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5c4f80), body.getX(), body.getY(),
                                             juce::Colour (0xff1a1230), body.getX(), body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (body.reduced (0.5f), 1.f);

    // indicador
    const auto p1 = c.getPointOnCircumference (radius * 0.56f, aTo);
    const auto p2 = c.getPointOnCircumference (radius * 0.20f, aTo);
    g.setColour (juce::Colours::white.withAlpha (en ? 1.f : 0.4f));
    g.drawLine (juce::Line<float> (p2, p1), 2.2f);
}

void CyberLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                         float minPos, float maxPos, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
        return;
    }

    const float cy = (float) y + (float) h * 0.5f;
    const auto neon = s.findColour (juce::Slider::trackColourId);

    // ranura hundida
    g.setColour (juce::Colour (0xff090510));
    g.fillRoundedRectangle ((float) x, cy - 3.f, (float) w, 6.f, 3.f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine ((float) x + 2.f, cy + 3.5f, (float) (x + w) - 2.f, cy + 3.5f, 1.f);

    g.setColour (neon.withAlpha (0.25f));
    g.fillRoundedRectangle ((float) x, cy - 6.f, sliderPos - (float) x, 12.f, 5.f);
    g.setColour (neon);
    g.fillRoundedRectangle ((float) x, cy - 2.5f, sliderPos - (float) x, 5.f, 2.5f);

    // deslizador con relieve
    auto thumb = juce::Rectangle<float> (10.f, 18.f).withCentre ({ sliderPos, cy });
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (thumb.translated (0.f, 1.5f), 3.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8d0f0), 0.f, thumb.getY(),
                                             juce::Colour (0xff6a5f90), 0.f, thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.f);
}

void CyberLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (1.5f);
    auto p = Cyber::chamfer (r, 5.f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillPath (Cyber::chamfer (r.translated (0.f, 1.5f), 5.f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2044), 0.f, r.getY(),
                                             juce::Colour (0xff110a1e), 0.f, r.getBottom(), false));
    g.fillPath (p);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (r.getX() + 7.f, r.getY() + 1.2f, r.getRight() - 2.f, r.getY() + 1.2f, 1.f);
    g.setColour (Cyber::cyan.withAlpha (0.65f));
    g.strokePath (p, juce::PathStrokeType (1.2f));

    juce::Path arrow;
    const float ax = (float) w - 22.f, ay = (float) h * 0.5f - 3.f;
    arrow.addTriangle (ax, ay, ax + 10.f, ay, ax + 5.f, ay + 6.f);
    g.setColour (Cyber::cyan);
    g.fillPath (arrow);
}

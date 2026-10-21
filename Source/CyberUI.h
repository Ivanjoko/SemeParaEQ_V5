#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
//  Estilo cyberpunk con relieve: colores neón, biseles, sombras y brillos
//==============================================================================
namespace Cyber
{
    inline const juce::Colour bg      { 0xff07050f };
    inline const juce::Colour panel   { 0xff110a1f };
    inline const juce::Colour dim     { 0xff2a1f44 };
    inline const juce::Colour cyan    { 0xff00f0ff };
    inline const juce::Colour magenta { 0xffff2bd6 };
    inline const juce::Colour yellow  { 0xfffcee0a };
    inline const juce::Colour green   { 0xff39ff14 };
    inline const juce::Colour orange  { 0xffff7a00 };
    inline const juce::Colour violet  { 0xffb026ff };

    inline juce::Colour band (int b)
    {
        static const juce::Colour c[6] = { cyan, magenta, yellow, green, orange, violet };
        return c[((b % 6) + 6) % 6];
    }

    // Rectángulo con las esquinas superior izquierda e inferior derecha cortadas
    inline juce::Path chamfer (juce::Rectangle<float> r, float c)
    {
        juce::Path p;
        p.startNewSubPath (r.getX() + c, r.getY());
        p.lineTo (r.getRight(), r.getY());
        p.lineTo (r.getRight(), r.getBottom() - c);
        p.lineTo (r.getRight() - c, r.getBottom());
        p.lineTo (r.getX(), r.getBottom());
        p.lineTo (r.getX(), r.getY() + c);
        p.closeSubPath();
        return p;
    }

    // Tipografía técnica y limpia: DIN en macOS; si no existe, la primera disponible de la lista
    inline const juce::String& fontName()
    {
        static const juce::String name = []
        {
            const auto all = juce::Font::findAllTypefaceNames();
            for (auto* n : { "DIN Alternate", "Avenir Next", "Helvetica Neue", "Segoe UI" })
                if (all.contains (n))
                    return juce::String (n);
            return juce::Font::getDefaultSansSerifFontName();
        }();
        return name;
    }

    inline juce::FontOptions font (float h, bool bold = false)
    {
        return juce::FontOptions (fontName(), h, bold ? juce::Font::bold : juce::Font::plain);
    }

    inline juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    // Panel con relieve: sombra, degradado, luz superior, brillo neón y borde
    void drawBevelPanel (juce::Graphics&, juce::Rectangle<float>, juce::Colour neon, float cut = 12.f);
}

//==============================================================================
//  Botón con relieve y animación de pulsación (se hunde al apretarlo)
//==============================================================================
class CyberButton : public juce::Button, private juce::Timer
{
public:
    explicit CyberButton (const juce::String& name = {}) : juce::Button (name) {}

    void setNeon (juce::Colour c)  { neon = c; repaint(); }
    void setArrow (float dir)      { arrowDir = dir; hasArrow = true; repaint(); }   // 0 = derecha, .25 = abajo, .5 = izquierda, .75 = arriba
    void setSmall (bool s)         { small = s; }

    void paintButton (juce::Graphics&, bool over, bool down) override;
    void buttonStateChanged() override { if (! isTimerRunning()) startTimerHz (60); }

private:
    void timerCallback() override;

    juce::Colour neon { 0xff00f0ff };
    float arrowDir = 0.f, press = 0.f, hover = 0.f;
    bool hasArrow = false, small = false;
};

//==============================================================================
//  Contenedor con panel en relieve (se dibuja una sola vez en una imagen)
//==============================================================================
class Region : public juce::Component
{
public:
    explicit Region (bool framedPanel = false, juce::Colour neonColour = Cyber::cyan)
        : framed (framedPanel), neon (neonColour) {}

    void setFullSize (int w, int h);                 // tamaño completo (también cambia el tamaño del componente)
    void paint (juce::Graphics&) override;
    std::function<void (juce::Graphics&)> paintOver; // contenido dinámico (títulos, etc.)

private:
    bool framed;
    juce::Colour neon;
    juce::Image cache;
};

//==============================================================================
class CyberLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CyberLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
};

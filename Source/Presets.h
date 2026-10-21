#pragma once
#include "PluginProcessor.h"

//==============================================================================
//  Presets de fábrica y presets del usuario
//==============================================================================
namespace Presets
{
    struct Band
    {
        int type;
        float freq, gain, q;
        bool dyn = false;
        float dRange = -6.f, dThr = -30.f, dAtk = 10.f, dRel = 120.f;
    };

    struct Preset
    {
        const char* category;
        const char* name;
        bool hpf;  float hpfF;  int hpfSlope;      // slope: 0 = 12, 1 = 24, 2 = 36, 3 = 48 dB/oct
        bool lpf;  float lpfF;  int lpfSlope;
        std::vector<Band> bands;
    };

    inline const std::vector<Preset>& factory()
    {
        using namespace EqMath;
        static const std::vector<Preset> list = {
            // ---- Filtros
            { "Filtros", "HPF 80 Hz - limpiar graves",        true, 80.f, 1,  false, 0.f, 1,  {} },
            { "Filtros", "HPF 30 Hz - quitar subsónicos",     true, 30.f, 2,  false, 0.f, 1,  {} },
            { "Filtros", "LPF 16 kHz - suavizar agudos",      false, 0.f, 1,  true, 16000.f, 1, {} },
            { "Filtros", "LPF 8 kHz - sonido apagado",        false, 0.f, 1,  true, 8000.f, 1,  {} },

            // ---- Efectos creativos
            { "Efectos", "Llamada de teléfono",  true, 300.f, 2,  true, 3400.f, 2,  { { Bell, 1500.f, 3.f, 0.8f } } },
            { "Efectos", "Radio AM",             true, 500.f, 1,  true, 2800.f, 1,  { { Bell, 1500.f, 4.f, 1.5f } } },
            { "Efectos", "Megáfono",             true, 400.f, 2,  true, 4000.f, 2,  { { Bell, 1800.f, 6.f, 1.5f } } },
            { "Efectos", "Lo-fi / vinilo",       true, 150.f, 0,  true, 6000.f, 1,  { { Bell, 800.f, 3.f, 0.8f } } },
            { "Efectos", "Bajo el agua",         false, 0.f, 1,   true, 700.f, 2,   {} },

            // ---- Voz
            { "Voz", "Voz clara",          true, 80.f, 1,  false, 0.f, 1,
                { { Bell, 250.f, -3.f, 1.2f }, { Bell, 3000.f, 2.f, 1.f }, { HighShelf, 10000.f, 2.f, 0.7f } } },
            { "Voz", "Voz masculina",      true, 80.f, 1,  false, 0.f, 1,
                { { Bell, 200.f, -2.f, 1.f }, { Bell, 3500.f, 3.f, 1.f }, { HighShelf, 10000.f, 2.f, 0.7f } } },
            { "Voz", "Voz femenina",       true, 120.f, 1, false, 0.f, 1,
                { { Bell, 300.f, -2.f, 1.f }, { Bell, 5000.f, 2.5f, 1.f }, { HighShelf, 12000.f, 2.f, 0.7f } } },
            { "Voz", "Podcast / locución", true, 80.f, 2,  false, 0.f, 1,
                { { Bell, 120.f, 2.f, 0.8f }, { Bell, 250.f, -2.f, 1.2f }, { Bell, 4000.f, 3.f, 1.f },
                  { Bell, 7000.f, 0.f, 3.f, true, -6.f, -30.f, 1.f, 80.f } } },
            { "Voz", "Presencia vocal",    false, 0.f, 1,  false, 0.f, 1,
                { { Bell, 4000.f, 3.f, 1.f }, { Bell, 200.f, -2.f, 1.f } } },
            { "Voz", "De-esser (dinámico)", false, 0.f, 1,  false, 0.f, 1,
                { { Bell, 7000.f, 0.f, 3.f, true, -8.f, -30.f, 1.f, 80.f } } },
            { "Voz", "Quitar barro",       false, 0.f, 1,  false, 0.f, 1,  { { Bell, 250.f, -3.f, 1.2f } } },

            // ---- Percusión
            { "Percusión", "Bombo (kick)",          true, 30.f, 1,  false, 0.f, 1,
                { { Bell, 60.f, 3.f, 1.2f }, { Bell, 350.f, -4.f, 1.5f }, { Bell, 3500.f, 3.f, 1.f } } },
            { "Percusión", "Caja (snare)",          true, 80.f, 1,  false, 0.f, 1,
                { { Bell, 200.f, 2.f, 1.f }, { Bell, 500.f, -2.f, 1.5f }, { Bell, 5000.f, 3.f, 1.f }, { HighShelf, 10000.f, 1.5f, 0.7f } } },
            { "Percusión", "Hi-hat y platillos",    true, 300.f, 1, false, 0.f, 1,
                { { Bell, 8000.f, 2.f, 1.f }, { HighShelf, 12000.f, 2.f, 0.7f } } },
            { "Percusión", "Batería (overheads)",   true, 200.f, 1, false, 0.f, 1,
                { { Bell, 400.f, -2.f, 1.f }, { HighShelf, 10000.f, 2.f, 0.7f } } },

            // ---- Cuerdas y teclas
            { "Cuerdas y teclas", "Guitarra eléctrica",  true, 90.f, 1,  true, 10000.f, 1,
                { { Bell, 300.f, -2.f, 1.f }, { Bell, 2500.f, 3.f, 1.2f } } },
            { "Cuerdas y teclas", "Guitarra acústica",   true, 80.f, 1,  false, 0.f, 1,
                { { Bell, 200.f, -2.f, 1.f }, { Bell, 5000.f, 2.f, 1.f }, { HighShelf, 12000.f, 2.f, 0.7f } } },
            { "Cuerdas y teclas", "Piano",               true, 40.f, 1,  false, 0.f, 1,
                { { Bell, 300.f, -2.f, 1.f }, { Bell, 3500.f, 2.f, 1.f }, { HighShelf, 10000.f, 1.f, 0.7f } } },
            { "Cuerdas y teclas", "Cuerdas (violín)",    true, 150.f, 1, false, 0.f, 1,
                { { Bell, 250.f, -2.f, 1.f }, { Bell, 3000.f, 2.f, 0.9f }, { HighShelf, 9000.f, 1.5f, 0.7f } } },

            // ---- Bajo y sintetizadores
            { "Bajo y sintes", "Bajo eléctrico",     true, 35.f, 1,  false, 0.f, 1,
                { { Bell, 80.f, 3.f, 1.f }, { Bell, 250.f, -3.f, 1.5f }, { Bell, 900.f, 2.f, 1.2f } } },
            { "Bajo y sintes", "Bajo 808",           true, 25.f, 1,  false, 0.f, 1,
                { { LowShelf, 60.f, 3.f, 0.7f }, { Bell, 250.f, -3.f, 1.5f } } },
            { "Bajo y sintes", "Sintetizador / pad", true, 100.f, 1, false, 0.f, 1,
                { { Bell, 400.f, -2.f, 1.f }, { HighShelf, 9000.f, 2.f, 0.7f } } },

            // ---- Viento
            { "Viento", "Saxo / flauta",   true, 150.f, 1,  false, 0.f, 1,
                { { Bell, 500.f, -2.f, 1.f }, { Bell, 4000.f, 2.f, 1.f } } },
            { "Viento", "Trompeta / metales", true, 150.f, 1, false, 0.f, 1,
                { { Bell, 400.f, -2.f, 1.f }, { Bell, 3000.f, 2.f, 1.f }, { HighShelf, 10000.f, 1.f, 0.7f } } },

            // ---- Música / mastering
            { "Música", "Más graves",        false, 0.f, 1,  false, 0.f, 1,  { { LowShelf, 90.f, 4.f, 0.7f } } },
            { "Música", "Brillo y aire",     false, 0.f, 1,  false, 0.f, 1,  { { HighShelf, 12000.f, 3.f, 0.7f } } },
            { "Música", "Mastering suave",   true, 25.f, 0,  false, 0.f, 1,
                { { LowShelf, 80.f, 1.f, 0.7f }, { Bell, 300.f, -1.f, 1.f }, { HighShelf, 11000.f, 1.5f, 0.7f } } },

            // ---- Corrección
            { "Corrección", "Quitar zumbido 50 Hz", false, 0.f, 1, false, 0.f, 1,
                { { Notch, 50.f, 0.f, 10.f }, { Notch, 100.f, 0.f, 10.f }, { Notch, 150.f, 0.f, 10.f } } },
            { "Corrección", "Quitar zumbido 60 Hz", false, 0.f, 1, false, 0.f, 1,
                { { Notch, 60.f, 0.f, 10.f }, { Notch, 120.f, 0.f, 10.f }, { Notch, 180.f, 0.f, 10.f } } },
        };
        return list;
    }

    //---- Utilidades para cambiar parámetros (con gestos, para que el DAW los vea)
    inline void setReal (juce::AudioProcessorValueTreeState& a, const juce::String& id, float v)
    {
        if (auto* p = a.getParameter (id))
        {
            const float n = p->convertTo0to1 (v);
            if (std::abs (p->getValue() - n) > 1e-7f)
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (n);
                p->endChangeGesture();
            }
        }
    }

    inline void setDefault (juce::AudioProcessorValueTreeState& a, const juce::String& id)
    {
        if (auto* p = a.getParameter (id))
        {
            const float d = p->getDefaultValue();
            if (std::abs (p->getValue() - d) > 1e-7f)
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (d);
                p->endChangeGesture();
            }
        }
    }

    // Borra todas las bandas y los filtros; con alsoGlobals también entrada, salida, polaridad y bypass
    inline void clearEq (juce::AudioProcessorValueTreeState& a, bool alsoGlobals)
    {
        for (int b = 0; b < EqMath::numBands; ++b)
        {
            for (auto* s : { "on", "type", "freq", "gain", "q", "dyn", "dRange", "dThr", "dAtk", "dRel" })
                setDefault (a, bandId (b, s));
            setReal (a, bandId (b, "used"), 0.f);
        }
        for (auto* s : { "hpfOn", "hpfFreq", "hpfSlope", "lpfOn", "lpfFreq", "lpfSlope" })
            setDefault (a, s);
        if (alsoGlobals)
            for (auto* s : { "inGain", "outGain", "bypass", "polarity" })
                setDefault (a, s);
    }

    inline void apply (juce::AudioProcessorValueTreeState& a, const Preset& p)
    {
        clearEq (a, false);

        setReal (a, "hpfOn", p.hpf ? 1.f : 0.f);
        if (p.hpf) { setReal (a, "hpfFreq", p.hpfF); setReal (a, "hpfSlope", (float) p.hpfSlope); }
        setReal (a, "lpfOn", p.lpf ? 1.f : 0.f);
        if (p.lpf) { setReal (a, "lpfFreq", p.lpfF); setReal (a, "lpfSlope", (float) p.lpfSlope); }

        int i = 0;
        for (const auto& b : p.bands)
        {
            if (i >= EqMath::numBands) break;
            setReal (a, bandId (i, "type"),   (float) b.type);
            setReal (a, bandId (i, "freq"),   b.freq);
            setReal (a, bandId (i, "gain"),   b.gain);
            setReal (a, bandId (i, "q"),      b.q);
            setReal (a, bandId (i, "dyn"),    b.dyn ? 1.f : 0.f);
            setReal (a, bandId (i, "dRange"), b.dRange);
            setReal (a, bandId (i, "dThr"),   b.dThr);
            setReal (a, bandId (i, "dAtk"),   b.dAtk);
            setReal (a, bandId (i, "dRel"),   b.dRel);
            setReal (a, bandId (i, "on"),     1.f);
            setReal (a, bandId (i, "used"),   1.f);
            ++i;
        }
    }

    //---- Presets del usuario (archivos .paraeq)
    inline juce::File userDir()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("SemeParaEQ").getChildFile ("Presets");
    }

    inline std::vector<juce::File> userFiles()
    {
        auto arr = userDir().findChildFiles (juce::File::findFiles, false, "*.paraeq");
        std::vector<juce::File> v (arr.begin(), arr.end());
        std::sort (v.begin(), v.end(), [] (const juce::File& x, const juce::File& y)
                   { return x.getFileName().compareIgnoreCase (y.getFileName()) < 0; });
        return v;
    }

    inline bool saveUser (juce::AudioProcessorValueTreeState& a, const juce::String& name)
    {
        auto dir = userDir();
        if (! dir.createDirectory().wasOk())
            return false;

        auto file = dir.getChildFile (juce::File::createLegalFileName (name) + ".paraeq");
        if (auto xml = a.copyState().createXml())
            return xml->writeTo (file);
        return false;
    }

    inline void applyXml (juce::AudioProcessorValueTreeState& a, const juce::XmlElement& xml)
    {
        for (auto* child : xml.getChildIterator())
            if (child->hasTagName ("PARAM"))
                setReal (a, child->getStringAttribute ("id"), (float) child->getDoubleAttribute ("value"));
    }
}

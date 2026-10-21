#pragma once
#include "PluginProcessor.h"
#include "CyberUI.h"

//==============================================================================
//  Vúmetro estéreo de LEDs (L y R)
//==============================================================================
class StereoMeter : public juce::Component
{
public:
    explicit StereoMeter (juce::String t) : title (std::move (t)) { setOpaque (true); }

    void setLevels (float rmsL, float rmsR, float pkL, float pkR);
    void paint (juce::Graphics&) override;

private:
    juce::String title;
    float rms[2] { -100.f, -100.f }, pk[2] { -100.f, -100.f };
};

//==============================================================================
//  Panel de información (botón INFO)
//==============================================================================
class InfoOverlay : public juce::Component
{
public:
    InfoOverlay();
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void()> onClose;

private:
    juce::TextEditor text;
    CyberButton closeBtn { "CERRAR" };
};

//==============================================================================
//  Pantalla interactiva
//   - doble clic en vacío: añadir banda · arrastrar en vacío: selección rectangular
//   - arrastrar un punto: mueve todas las bandas seleccionadas (Alt = ajuste fino)
//   - Shift/Cmd-clic: suma o quita bandas de la selección
//   - rueda sobre un punto: Q · rueda en vacío: zoom en frecuencia
//   - Alt + rueda: zoom vertical · Alt + arrastrar (o botón central): mover la vista
//==============================================================================
class EqGraph : public juce::Component, private juce::Timer
{
public:
    EqGraph (ParaEQProcessor&, std::function<void (int)> onSelectBand);
    ~EqGraph() override;

    void setRange (int db);
    void resetView();
    void setSpectrum (bool on)   { showSpectrum = on; specDirty = true; repaint(); }
    void setResonances (bool on) { showRes = on; repaint(); }
    void setPaused (bool p)      { paused = p; repaint(); }

    void setSelected (int b);                    // selección única
    void refreshSelection();                     // quita de la selección las bandas que ya no existen
    void selectNeighbor (int dir);               // -1 = banda anterior (más grave), +1 = siguiente
    int  addBand (float freq, float gainDb, int type, float q);
    void deleteBand (int b);
    void deleteSelected();
    int  firstUsed() const;
    int  numUsed() const;
    bool bandUsed (int b) const { return b >= 0 && b < EqMath::numBands && proc.pBand[(size_t) b].used->load() > 0.5f; }
    int  primary() const { return selected; }
    int  selectedCount() const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;

private:
    struct View { double lo, hi, center, half; };      // lo/hi: log(Hz) · center/half: dB
    struct Resonance { float freq, excess; };
    struct Band { bool used, on, dyn; int type; float freq, gain, q; };
    enum class Drag { None, Node, Marquee, Pan };

    void timerCallback() override;
    bool stepView();
    void clampView (View&) const;
    void zoomFreq (float x, double factor);
    void zoomGain (float y, double factor);
    void ensureVisible (float freq);

    bool updateSpectrum();
    void rebuildSpectrum();
    void rebuildCurve();
    void ensureGrid();
    void findResonances();
    juce::uint32 paramHash() const;

    void showMenu (int band);
    void setSelection (juce::uint32 mask, int prim);
    juce::uint32 usedMask() const;

    Band band (int b) const;
    juce::Rectangle<float> plot() const;
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float gainToY (float g) const;
    float yToGain (float y) const;
    juce::Point<float> nodePos (int b) const;
    bool nodeVisible (juce::Point<float> p) const;
    int  hitTest (juce::Point<float>) const;
    bool isSel (int b) const { return (selMask >> b) & 1u; }

    void setNorm (juce::RangedAudioParameter* p, float real) { p->setValueNotifyingHost (p->convertTo0to1 (real)); }
    void editParam (juce::RangedAudioParameter* p, float real);

    ParaEQProcessor& proc;
    std::function<void (int)> onSelect;

    View cur { std::log (20.0), std::log (20000.0), 0.0, 18.0 }, tgt = cur;
    int selected = -1;
    juce::uint32 selMask = 0;
    bool showSpectrum = true, showRes = false, paused = false, specDirty = true, curveDirty = true;
    juce::uint32 lastHash = 0;

    // interacción
    Drag drag = Drag::None;
    int dragBand = -1;
    bool lastFine = false, animating = false;
    juce::Point<float> anchorMouse, anchorNode, panStart, marqueeStart;
    View panStartView { 0, 0, 0, 0 };
    juce::Rectangle<float> marquee;
    juce::uint32 marqueeBase = 0;
    float startF[EqMath::numBands] {}, startG[EqMath::numBands] {};
    std::vector<juce::RangedAudioParameter*> gestures;

    // gráficos en caché
    juce::Image gridImg;
    View gridView { 0, 0, 0, 0 };
    juce::Path curvePath, selFill, specFill, specLine;

    // análisis de espectro
    static constexpr int fftSize = ParaEQProcessor::fftSize;
    juce::dsp::FFT fft { ParaEQProcessor::fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, 2 * fftSize> fftData {};
    std::array<float, fftSize / 2> spec {}, specSlow {};
    float lastSpecSum = 0.f;
    std::vector<Resonance> res;
};

//==============================================================================
class ParaEQEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ParaEQEditor (ParaEQProcessor&);
    ~ParaEQEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    struct FnTimer : juce::Timer
    {
        std::function<void()> fn;
        void timerCallback() override { if (fn) fn(); }
    };

    void timerCallback() override;
    void layoutMain();
    void layoutTop();
    void layoutBottom();
    void stepPanels();
    void togglePanel (int which);

    void selectBand (int b);
    void updateEnabled();
    void knob (juce::Slider&);
    void label (juce::Label&, const juce::String&);
    void runProgrammatic (std::function<void()> change);
    void setPresetName (const juce::String&);
    void showPresetMenu();
    void askSavePreset();
    void loadPresetFile (const juce::File&);

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    static constexpr int topFull = 96, botFull = 270, meterFull = 84;

    CyberLookAndFeel laf;
    ParaEQProcessor& proc;
    EqGraph graph;
    StereoMeter meterIn { "IN" }, meterOut { "OUT" };
    InfoOverlay info;

    // zonas que se pueden minimizar
    Region topRegion, bottomRegion, leftRegion, rightRegion;
    Region filtersPanel { true }, bandPanel { true };

    juce::Label lblIn, lblOut, lblFreq, lblGain, lblQ, lblRange, lblThr, lblAtk, lblRel, lblHpf, lblLpf;
    juce::Slider inSl, outSl, hpfFreq, lpfFreq, bFreq, bGain, bQ, dRange, dThr, dAtk, dRel;
    CyberButton bypassBtn, polBtn, analyzerBtn, resBtn, infoBtn, hpfOn, lpfOn, bandOn, dynBtn, addBtn, delBtn;
    CyberButton presetBtn, saveBtn, clearBtn, centerBtn, pauseBtn, undoBtn, redoBtn;
    CyberButton handle[4];                      // minimizar: 0 arriba, 1 abajo, 2 izquierda, 3 derecha
    juce::ComboBox hpfSlope, lpfSlope, bType, rangeBox;
    juce::HyperlinkButton link { "Instagram  @Ivanjoko_",
        juce::URL ("https://www.instagram.com/ivanjoko_?exln=MWpsYXhmODV0enQ1bA%3D%3D&utm_source=qr") };

    std::unique_ptr<SA> aIn, aOut, aHpfFreq, aLpfFreq, aBFreq, aBGain, aBQ, aDRange, aDThr, aDAtk, aDRel;
    std::unique_ptr<BA> aBypass, aPol, aHpfOn, aLpfOn, aBandOn, aDyn;
    std::unique_ptr<CA> aHpfSlope, aLpfSlope, aBType;

    float vis[4] { 1.f, 1.f, 1.f, 1.f }, visTarget[4] { 1.f, 1.f, 1.f, 1.f };
    FnTimer animTimer;

    int selected = -1;
    float inR[2] {}, inP[2] {}, outR[2] {}, outP[2] {};
    juce::Rectangle<int> capFilter, capDyn, footer;
    juce::Image bgCache;
    juce::String currentPreset;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParaEQEditor)
};

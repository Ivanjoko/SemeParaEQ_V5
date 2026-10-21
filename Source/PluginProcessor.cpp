#include "PluginProcessor.h"
#include "PluginEditor.h"

ParaEQProcessor::ParaEQProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto& s : scope) s.store (0.0f);
    for (auto& s : inPk)  s.store (0.0f);
    for (auto& s : inMs)  s.store (0.0f);
    for (auto& s : outPk) s.store (0.0f);
    for (auto& s : outMs) s.store (0.0f);
    for (auto& s : dynShown) s.store (0.0f);

    auto raw = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    auto par = [this] (const juce::String& id) { return apvts.getParameter (id); };

    pIn = raw ("inGain");     pOut = raw ("outGain");     pBypass = raw ("bypass");  pPolarity = raw ("polarity");
    pHpfOn = raw ("hpfOn");   pHpfFreq = raw ("hpfFreq"); pHpfSlope = raw ("hpfSlope");
    pLpfOn = raw ("lpfOn");   pLpfFreq = raw ("lpfFreq"); pLpfSlope = raw ("lpfSlope");

    for (int b = 0; b < EqMath::numBands; ++b)
    {
        pBand[(size_t) b] = { raw (bandId (b, "used")), raw (bandId (b, "on")), raw (bandId (b, "type")),
                              raw (bandId (b, "freq")), raw (bandId (b, "gain")), raw (bandId (b, "q")),
                              raw (bandId (b, "dyn")), raw (bandId (b, "dRange")), raw (bandId (b, "dThr")),
                              raw (bandId (b, "dAtk")), raw (bandId (b, "dRel")) };

        bandParam[(size_t) b] = { par (bandId (b, "used")), par (bandId (b, "on")), par (bandId (b, "type")),
                                  par (bandId (b, "freq")), par (bandId (b, "gain")), par (bandId (b, "q")),
                                  par (bandId (b, "dyn")), par (bandId (b, "dRange")), par (bandId (b, "dThr")),
                                  par (bandId (b, "dAtk")), par (bandId (b, "dRel")) };
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ParaEQProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;
    using Attr = juce::AudioParameterFloatAttributes;

    auto hz = [] (float v, int)
    {
        if (v >= 1000.f) return juce::String (v / 1000.f, 3) + " kHz";
        if (v >= 100.f)  return juce::String (v, 1) + " Hz";
        return juce::String (v, 2) + " Hz";
    };
    auto hzParse = [] (const juce::String& t)
    {
        auto x = t.trim().toLowerCase();
        float v = x.getFloatValue();
        if (x.contains ("k")) v *= 1000.f;
        return v;
    };
    auto db      = [] (float v, int) { return (v > 0.f ? "+" : "") + juce::String (v, 2) + " dB"; };
    auto dbParse = [] (const juce::String& t) { return t.getFloatValue(); };
    auto qTxt    = [] (float v, int) { return juce::String (v, 2); };
    auto qParse  = [] (const juce::String& t) { return t.getFloatValue(); };
    auto ms      = [] (float v, int) { return juce::String (v, v < 10.f ? 1 : 0) + " ms"; };
    auto msParse = [] (const juce::String& t) { return t.getFloatValue(); };

    auto freqRange = [] (float lo, float hi, float centre)
    {
        R r (lo, hi, 0.01f);
        r.setSkewForCentre (centre);
        return r;
    };
    auto id = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };

    const juce::StringArray slopes { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };
    const juce::StringArray types  { "Campana", "Shelf graves", "Shelf agudos", "Notch" };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<F> (id ("inGain"),  "Entrada", R (-24.f, 24.f, 0.01f), 0.f,
                                     Attr().withStringFromValueFunction (db).withValueFromStringFunction (dbParse)));
    layout.add (std::make_unique<F> (id ("outGain"), "Salida",  R (-24.f, 24.f, 0.01f), 0.f,
                                     Attr().withStringFromValueFunction (db).withValueFromStringFunction (dbParse)));
    layout.add (std::make_unique<B> (id ("bypass"),   "Bypass", false));
    layout.add (std::make_unique<B> (id ("polarity"), "Invertir polaridad", false));

    layout.add (std::make_unique<B> (id ("hpfOn"), "HPF On", false));
    layout.add (std::make_unique<F> (id ("hpfFreq"), "HPF Freq", freqRange (20.f, 1000.f, 120.f), 30.f,
                                     Attr().withStringFromValueFunction (hz).withValueFromStringFunction (hzParse)));
    layout.add (std::make_unique<C> (id ("hpfSlope"), "HPF Slope", slopes, 1));

    layout.add (std::make_unique<B> (id ("lpfOn"), "LPF On", false));
    layout.add (std::make_unique<F> (id ("lpfFreq"), "LPF Freq", freqRange (1000.f, 20000.f, 5000.f), 18000.f,
                                     Attr().withStringFromValueFunction (hz).withValueFromStringFunction (hzParse)));
    layout.add (std::make_unique<C> (id ("lpfSlope"), "LPF Slope", slopes, 1));

    // El plugin arranca sin ninguna banda: se añaden con doble clic en la gráfica
    for (int b = 0; b < EqMath::numBands; ++b)
    {
        const juce::String n = "Banda " + juce::String (b + 1) + " ";

        layout.add (std::make_unique<B> (id (bandId (b, "used")), n + "Existe", false));
        layout.add (std::make_unique<B> (id (bandId (b, "on")),   n + "On", true));
        layout.add (std::make_unique<C> (id (bandId (b, "type")), n + "Tipo", types, (int) EqMath::Bell));
        layout.add (std::make_unique<F> (id (bandId (b, "freq")), n + "Freq", freqRange (20.f, 20000.f, 1000.f), 1000.f,
                                         Attr().withStringFromValueFunction (hz).withValueFromStringFunction (hzParse)));
        layout.add (std::make_unique<F> (id (bandId (b, "gain")), n + "Ganancia", R (-24.f, 24.f, 0.01f), 0.f,
                                         Attr().withStringFromValueFunction (db).withValueFromStringFunction (dbParse)));
        R qr (0.1f, 18.f, 0.01f);
        qr.setSkewForCentre (1.f);
        layout.add (std::make_unique<F> (id (bandId (b, "q")), n + "Q", qr, 1.0f,
                                         Attr().withStringFromValueFunction (qTxt).withValueFromStringFunction (qParse)));

        layout.add (std::make_unique<B> (id (bandId (b, "dyn")), n + "Dinamica", false));
        layout.add (std::make_unique<F> (id (bandId (b, "dRange")), n + "Din Rango", R (-24.f, 24.f, 0.1f), -6.f,
                                         Attr().withStringFromValueFunction (db).withValueFromStringFunction (dbParse)));
        layout.add (std::make_unique<F> (id (bandId (b, "dThr")), n + "Din Umbral", R (-60.f, 0.f, 0.1f), -30.f,
                                         Attr().withStringFromValueFunction (db).withValueFromStringFunction (dbParse)));
        R atk (0.1f, 200.f, 0.1f);  atk.setSkewForCentre (20.f);
        layout.add (std::make_unique<F> (id (bandId (b, "dAtk")), n + "Din Ataque", atk, 10.f,
                                         Attr().withStringFromValueFunction (ms).withValueFromStringFunction (msParse)));
        R rel (5.f, 2000.f, 1.f);  rel.setSkewForCentre (200.f);
        layout.add (std::make_unique<F> (id (bandId (b, "dRel")), n + "Din Release", rel, 120.f,
                                         Attr().withStringFromValueFunction (ms).withValueFromStringFunction (msParse)));
    }
    return layout;
}

//==============================================================================
//  Historial de deshacer / rehacer
//==============================================================================
ParaEQProcessor::Snapshot ParaEQProcessor::takeSnapshot() const
{
    Snapshot s;
    const auto& ps = getParameters();
    s.reserve ((size_t) ps.size());
    for (auto* p : ps)
        s.push_back (p->getValue());
    return s;
}

void ParaEQProcessor::restoreSnapshot (const Snapshot& s)
{
    const auto& ps = getParameters();
    for (int i = 0; i < ps.size() && i < (int) s.size(); ++i)
    {
        auto* p = ps[i];
        if (std::abs (p->getValue() - s[(size_t) i]) > 1e-7f)
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (s[(size_t) i]);
            p->endChangeGesture();
        }
    }
}

void ParaEQProcessor::flushPending()
{
    if (! dirty)
        return;

    const auto snap = takeSnapshot();
    if (snap != preEdit)
    {
        undoStack.push_back (preEdit);
        if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
        redoStack.clear();
    }
    committed = lastSnap = snap;
    dirty = false;
}

void ParaEQProcessor::historyTouch()
{
    touchMs = juce::Time::getMillisecondCounter();
}

void ParaEQProcessor::historyTick (bool rawInteracting)
{
    const auto now = juce::Time::getMillisecondCounter();
    if (rawInteracting) touchMs = now;
    const bool interacting = (now - touchMs) < 800u;

    const auto snap = takeSnapshot();
    if (committed.empty())
    {
        committed = lastSnap = snap;
        return;
    }

    if (snap != lastSnap)
    {
        if (interacting || dirty)               // cambio hecho por el usuario
        {
            if (! dirty) { dirty = true; preEdit = committed; }
            lastChange = now;
        }
        else
        {
            committed = snap;                   // cambio externo (automatización): no se guarda
        }
        lastSnap = snap;
    }

    if (dirty && ! interacting && now - lastChange > 350u)
        flushPending();
}

void ParaEQProcessor::historyCheckpoint()
{
    flushPending();
    undoStack.push_back (takeSnapshot());
    if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void ParaEQProcessor::historyAfterProgrammatic()
{
    committed = lastSnap = takeSnapshot();
    dirty = false;
}

void ParaEQProcessor::undo()
{
    flushPending();
    if (undoStack.empty()) return;

    const Snapshot target = undoStack.back();
    undoStack.pop_back();
    redoStack.push_back (takeSnapshot());
    restoreSnapshot (target);
    committed = lastSnap = takeSnapshot();
}

void ParaEQProcessor::redo()
{
    flushPending();
    if (redoStack.empty()) return;

    const Snapshot target = redoStack.back();
    redoStack.pop_back();
    undoStack.push_back (takeSnapshot());
    restoreSnapshot (target);
    committed = lastSnap = takeSnapshot();
}

//==============================================================================
void ParaEQProcessor::prepareToPlay (double sampleRate, int)
{
    currentSr.store (sampleRate);
    engine.prepare (sampleRate);
}

bool ParaEQProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

EqParams ParaEQProcessor::readParams() const
{
    EqParams p;
    p.inDb     = pIn->load();
    p.outDb    = pOut->load();
    p.bypass   = pBypass->load() > 0.5f;
    p.polarity = pPolarity->load() > 0.5f;

    p.hpf.on = pHpfOn->load() > 0.5f;  p.hpf.freq = pHpfFreq->load();  p.hpf.slope = (int) std::lround (pHpfSlope->load());
    p.lpf.on = pLpfOn->load() > 0.5f;  p.lpf.freq = pLpfFreq->load();  p.lpf.slope = (int) std::lround (pLpfSlope->load());

    for (int b = 0; b < EqMath::numBands; ++b)
    {
        const auto& s = pBand[(size_t) b];
        auto& d = p.band[(size_t) b];
        d.used   = s.used->load() > 0.5f;
        d.on     = s.on->load() > 0.5f;
        d.type   = (int) std::lround (s.type->load());
        d.freq   = s.freq->load();
        d.gainDb = s.gain->load();
        d.q      = s.q->load();
        d.dyn    = s.dyn->load() > 0.5f;
        d.dRange = s.dRange->load();
        d.dThr   = s.dThr->load();
        d.dAtk   = s.dAtk->load();
        d.dRel   = s.dRel->load();
    }
    return p;
}

template <typename T>
void ParaEQProcessor::process (juce::AudioBuffer<T>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() == 0)
        return;

    T* l = buffer.getWritePointer (0);
    T* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;   // mono: mismo canal

    const bool ui = uiOpen.load (std::memory_order_relaxed);
    engine.setMetering (ui);
    engine.setParams (readParams());
    engine.process (l, r, n);

    if (! ui)
        return;                                                            // sin editor: nada más que calcular

    const auto& mi = engine.getMeterIn();
    const auto& mo = engine.getMeterOut();
    for (size_t c = 0; c < 2; ++c)
    {
        inPk[c].store ((float) mi.pk[c]);   inMs[c].store ((float) mi.ms[c]);
        outPk[c].store ((float) mo.pk[c]);  outMs[c].store ((float) mo.ms[c]);
    }
    for (int b = 0; b < EqMath::numBands; ++b)
        dynShown[(size_t) b].store ((float) engine.getDynOffset (b), std::memory_order_relaxed);

    const int pos = scopePos.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
        scope[(size_t) ((pos + i) & (scopeSize - 1))].store (0.5f * (float) (l[i] + r[i]), std::memory_order_relaxed);
    scopePos.store ((pos + n) & (scopeSize - 1), std::memory_order_release);
}

void ParaEQProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)  { process (b); }
void ParaEQProcessor::processBlock (juce::AudioBuffer<double>& b, juce::MidiBuffer&) { process (b); }

juce::AudioProcessorEditor* ParaEQProcessor::createEditor()
{
    return new ParaEQEditor (*this);
}

void ParaEQProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void ParaEQProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParaEQProcessor();
}

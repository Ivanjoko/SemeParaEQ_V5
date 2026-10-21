#include "PluginEditor.h"
#include "Presets.h"

using EqMath::numBands;

static juce::String fmtHz (float f)
{
    return f >= 1000.f ? juce::String (f / 1000.f, 2) + " kHz" : juce::String ((int) std::lround (f)) + " Hz";
}

//==============================================================================
//  StereoMeter
//==============================================================================
void StereoMeter::setLevels (float rmsL, float rmsR, float pkL, float pkR)
{
    const float nr[2] = { rmsL, rmsR }, np[2] = { pkL, pkR };
    bool changed = false;
    for (int c = 0; c < 2; ++c)
        if (std::abs (nr[c] - rms[c]) > 0.2f || std::abs (np[c] - pk[c]) > 0.2f)
            changed = true;

    if (! changed)
        return;

    rms[0] = rmsL; rms[1] = rmsR; pk[0] = pkL; pk[1] = pkR;
    repaint();
}

void StereoMeter::paint (juce::Graphics& g)
{
    g.fillAll (Cyber::bg);

    auto b = getLocalBounds().toFloat().reduced (2.f);
    auto frame = Cyber::chamfer (b, 8.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1230), 0.f, b.getY(), juce::Colour (0xff0a0614), 0.f, b.getBottom(), false));
    g.fillPath (frame);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (b.getX() + 8.f, b.getY() + 1.5f, b.getRight() - 2.f, b.getY() + 1.5f, 1.f);
    g.setColour (Cyber::cyan.withAlpha (0.14f));
    g.strokePath (frame, juce::PathStrokeType (5.f));
    g.setColour (Cyber::cyan.withAlpha (0.65f));
    g.strokePath (frame, juce::PathStrokeType (1.2f));

    auto inner = b.reduced (7.f, 9.f);
    g.setColour (Cyber::cyan);
    g.setFont (Cyber::font (13.f, true));
    g.drawText (title, inner.removeFromTop (16.f).toNearestInt(), juce::Justification::centred);

    g.setColour (juce::Colours::grey);
    g.setFont (Cyber::font (11.f));
    g.drawText ("L      R", inner.removeFromBottom (16.f).toNearestInt(), juce::Justification::centred);

    constexpr int nSeg = 40;
    const float minDb = -60.f, maxDb = 6.f, step = (maxDb - minDb) / (float) nSeg;
    const float gap = 24.f;
    const float barW = (inner.getWidth() - gap) * 0.5f;
    const float segH = inner.getHeight() / (float) nSeg;

    for (int c = 0; c < 2; ++c)
    {
        const float x = inner.getX() + (float) c * (barW + gap);
        for (int s = 0; s < nSeg; ++s)
        {
            const float segDb = minDb + step * (float) (s + 1);
            const float y = inner.getBottom() - (float) (s + 1) * segH;
            const juce::Colour col = segDb < -18.f ? Cyber::green
                                   : segDb < -6.f  ? Cyber::yellow
                                   : segDb < 0.f   ? Cyber::orange : Cyber::magenta;

            const bool lit = rms[c] >= segDb - step;
            const bool isPeak = pk[c] >= segDb - step && pk[c] < segDb;

            g.setColour (isPeak ? juce::Colours::white : (lit ? col : col.withAlpha (0.10f)));
            g.fillRect (x, y + 1.f, barW, segH - 1.5f);
        }
    }

    g.setFont (Cyber::font (9.5f));
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    for (float d : { 0.f, -6.f, -12.f, -24.f, -48.f })
    {
        const float y = inner.getBottom() - (d - minDb) / (maxDb - minDb) * inner.getHeight();
        g.drawText (juce::String ((int) d), (int) (inner.getCentreX() - 12.f), (int) y - 5, 24, 10, juce::Justification::centred);
    }
}

//==============================================================================
//  InfoOverlay
//==============================================================================
static juce::String infoText()
{
    return juce::String::fromUTF8 (R"TXT(
SEMEPARAEQ · GUÍA RÁPIDA
════════════════════════

LA PANTALLA INTERACTIVA
• Doble clic en un espacio vacío: añade una banda nueva justo ahí (hasta 24). El plugin empieza sin ninguna.
• Arrastra un punto: izquierda y derecha cambian la frecuencia; arriba y abajo, la ganancia.
• Mantén ALT (Option en Mac) mientras arrastras un punto: ajuste 10 veces más fino.
• SELECCIONAR VARIAS BANDAS: arrastra un rectángulo sobre un espacio vacío, o haz clic en los puntos
  manteniendo SHIFT (o Cmd/Ctrl). Al mover uno de los seleccionados, se mueven todos juntos.
• Clic en un espacio vacío: quita la selección.
• Rueda del ratón sobre un punto: cambia la Q (lo ancha o estrecha que es la banda).
• Doble clic sobre un punto: pone su ganancia a 0 dB.
• Clic derecho sobre un punto: menú para eliminar, desactivar o activar la dinámica.

ZOOM Y DESPLAZAMIENTO
• Rueda en un espacio vacío (o pellizco en el trackpad): zoom en frecuencia, centrado donde está el cursor.
• ALT + rueda: zoom vertical (más o menos dB). El selector RANGO también cambia el zoom vertical.
• ALT + arrastrar en un espacio vacío (o botón central del ratón): mueve la vista en horizontal y vertical.
• Deslizar con dos dedos en horizontal: desplaza la vista a izquierda y derecha.
• CENTRAR VISTA: vuelve a la vista completa (20 Hz – 20 kHz).

TECLADO
• Flechas ◀ ▶: pasan a la banda anterior o siguiente (ordenadas de graves a agudos).
• Cmd + Z (Ctrl + Z en Windows): deshacer. Cmd + Shift + Z (Ctrl + Shift + Z o Ctrl + Y): rehacer.
• Suprimir / Retroceso: elimina las bandas seleccionadas.
Si tu programa de música se queda con las teclas, haz clic antes sobre la pantalla del plugin.

BARRA SUPERIOR
• ANALIZADOR: muestra u oculta el espectro de fondo. PAUSA: congela el espectro para estudiarlo con calma.
• RESONANCIAS: marca con triángulos las «frecuencias molestas»: picos que sobresalen del resto del sonido
  (zumbidos, silbidos, notas que «pinchan»). Haz clic en un triángulo y se crea una banda estrecha que la reduce.
• RANGO: cuántos dB se ven en la gráfica. INFO: esta ayuda.
• IN / OUT: volumen de entrada (antes del ecualizador) y de salida (después), en dB.
• Ø POL: invierte la polaridad (le da la vuelta a la onda). BYPASS: desactiva todo para comparar.

MINIMIZAR ZONAS
• Las pequeñas flechas en los bordes de la pantalla interactiva ocultan o muestran la barra superior,
  el panel inferior y cada vúmetro. Puedes dejar solo la pantalla interactiva.

DESHACER, PRESETS Y BORRAR TODO
• Flechas ◀ ▶ de la segunda fila: deshacer y rehacer (hasta 100 pasos).
• BORRAR TODO: quita todas las bandas y filtros. Si te arrepientes, deshaz.
• PRESETS: ajustes listos por categorías: filtros, efectos (teléfono, radio, megáfono…), voz, percusión,
  cuerdas y teclas, bajo y sintetizadores, viento, música y corrección de zumbidos.
• GUARDAR: guarda tu ajuste con el nombre que quieras; aparece en «Mis presets».

VÚMETROS
• IN (izquierda): nivel de entrada, canales L y R. OUT (derecha): nivel de salida, canales L y R.
• La barra muestra el nivel medio y la marca blanca, el pico. En magenta estás en 0 dB: cuidado con saturar.

PANEL DE BANDA (abajo a la derecha)
• Tipo: CAMPANA sube o baja una zona; SHELF GRAVES / AGUDOS sube o baja todo lo grave o todo lo agudo desde
  esa frecuencia; NOTCH corta una frecuencia muy concreta (no tiene ganancia).
• ACTIVA: enciende o apaga la banda sin borrarla. + BANDA / ELIMINAR: añade o borra bandas.
• FRECUENCIA, GANANCIA y Q: puedes escribir valores exactos con doble clic, por ejemplo «2.5k».

ECUALIZACIÓN DINÁMICA
Una banda dinámica solo actúa cuando esa frecuencia suena fuerte. Activa el botón DINÁMICA.
• RANGO: cuánto cambia la ganancia al superar el umbral (negativo = reduce, como un de-esser suave).
• UMBRAL: nivel a partir del cual actúa. ATAQUE y RELEASE: rapidez al actuar y al volver a la normalidad.

FILTROS DE CORTE
• HPF corta los graves por debajo de la frecuencia elegida. LPF corta los agudos por encima.
• La pendiente (12, 24, 36 o 48 dB/oct) indica lo brusco que es el corte.
)TXT");
}

InfoOverlay::InfoOverlay()
{
    text.setMultiLine (true, true);
    text.setReadOnly (true);
    text.setCaretVisible (false);
    text.setScrollbarsShown (true);
    text.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0a0614));
    text.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    text.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    text.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    text.setFont (juce::Font (Cyber::font (15.f)));
    text.setText (infoText());
    addAndMakeVisible (text);

    closeBtn.setNeon (Cyber::magenta);
    closeBtn.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeBtn);
}

void InfoOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.6f));
    Cyber::drawBevelPanel (g, getLocalBounds().toFloat().reduced (6.f), Cyber::magenta, 16.f);

    g.setColour (Cyber::cyan);
    g.setFont (Cyber::font (20.f, true));
    g.drawText ("// INFO", getLocalBounds().reduced (26, 14).removeFromTop (32), juce::Justification::centredLeft);
}

void InfoOverlay::resized()
{
    auto r = getLocalBounds().reduced (26, 16);
    auto head = r.removeFromTop (36);
    closeBtn.setBounds (head.removeFromRight (110).reduced (0, 2));
    r.removeFromTop (8);
    text.setBounds (r);
}

//==============================================================================
//  EqGraph: utilidades
//==============================================================================
EqGraph::EqGraph (ParaEQProcessor& p, std::function<void (int)> cb)
    : proc (p), onSelect (std::move (cb))
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    spec.fill (-120.f);
    specSlow.fill (-120.f);
    startTimerHz (30);
}

EqGraph::~EqGraph()
{
    stopTimer();
}

EqGraph::Band EqGraph::band (int b) const
{
    const auto& p = proc.pBand[(size_t) b];
    return { p.used->load() > 0.5f, p.on->load() > 0.5f, p.dyn->load() > 0.5f, (int) std::lround (p.type->load()),
             p.freq->load(), p.gain->load(), p.q->load() };
}

juce::Rectangle<float> EqGraph::plot() const
{
    return getLocalBounds().toFloat().reduced (14.f, 14.f).withTrimmedLeft (30.f).withTrimmedBottom (14.f);
}

float EqGraph::freqToX (float f) const
{
    auto r = plot();
    return (float) ((double) r.getX() + (double) r.getWidth() * (std::log ((double) f) - cur.lo) / (cur.hi - cur.lo));
}

float EqGraph::xToFreq (float x) const
{
    auto r = plot();
    return (float) std::exp (cur.lo + (double) (x - r.getX()) / (double) r.getWidth() * (cur.hi - cur.lo));
}

float EqGraph::gainToY (float g) const
{
    auto r = plot();
    return r.getBottom() - (float) (((double) g - (cur.center - cur.half)) / (2.0 * cur.half)) * r.getHeight();
}

float EqGraph::yToGain (float y) const
{
    auto r = plot();
    const double g = cur.center - cur.half + (double) (r.getBottom() - y) / (double) r.getHeight() * 2.0 * cur.half;
    return juce::jlimit (-24.f, 24.f, (float) g);
}

juce::Point<float> EqGraph::nodePos (int b) const
{
    const auto bd = band (b);
    const float g = (bd.type == EqMath::Notch) ? 0.f : bd.gain;
    auto pl = plot();
    return { freqToX (bd.freq), juce::jlimit (pl.getY(), pl.getBottom(), gainToY (g)) };
}

bool EqGraph::nodeVisible (juce::Point<float> p) const
{
    auto pl = plot();
    return p.x >= pl.getX() - 8.f && p.x <= pl.getRight() + 8.f;
}

int EqGraph::hitTest (juce::Point<float> p) const
{
    int best = -1;
    float bestD = 14.f;
    for (int b = 0; b < numBands; ++b)
    {
        if (! bandUsed (b)) continue;
        const auto np = nodePos (b);
        if (! nodeVisible (np)) continue;
        const float d = np.getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = b; }
    }
    return best;
}

void EqGraph::editParam (juce::RangedAudioParameter* p, float real)
{
    p->beginChangeGesture();
    setNorm (p, real);
    p->endChangeGesture();
}

juce::uint32 EqGraph::usedMask() const
{
    juce::uint32 m = 0;
    for (int b = 0; b < numBands; ++b)
        if (bandUsed (b)) m |= (1u << b);
    return m;
}

int EqGraph::firstUsed() const
{
    for (int b = 0; b < numBands; ++b)
        if (bandUsed (b)) return b;
    return -1;
}

int EqGraph::numUsed() const
{
    int n = 0;
    for (int b = 0; b < numBands; ++b)
        if (bandUsed (b)) ++n;
    return n;
}

int EqGraph::selectedCount() const
{
    int n = 0;
    for (int b = 0; b < numBands; ++b)
        if (isSel (b)) ++n;
    return n;
}

void EqGraph::setSelection (juce::uint32 mask, int prim)
{
    selMask = mask;
    if (mask == 0)
        prim = -1;
    else if (prim < 0 || ! isSel (prim))
        for (int b = 0; b < numBands; ++b)
            if (isSel (b)) { prim = b; break; }

    curveDirty = true;
    if (prim != selected)
    {
        selected = prim;
        onSelect (prim);
    }
    repaint();
}

void EqGraph::setSelected (int b)
{
    setSelection (b >= 0 ? (1u << b) : 0u, b);
}

void EqGraph::refreshSelection()
{
    const auto um = usedMask();
    if ((selMask & ~um) != 0u)
        setSelection (selMask & um, selected);
}

void EqGraph::selectNeighbor (int dir)
{
    std::vector<std::pair<float, int>> list;
    for (int b = 0; b < numBands; ++b)
        if (bandUsed (b)) list.push_back ({ band (b).freq, b });
    if (list.empty()) return;

    std::sort (list.begin(), list.end());

    int idx = -1;
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i].second == selected) idx = (int) i;

    const int n = (int) list.size();
    idx = (idx < 0) ? (dir > 0 ? 0 : n - 1) : ((idx + dir) % n + n) % n;

    const int b = list[(size_t) idx].second;
    ensureVisible (list[(size_t) idx].first);
    setSelection (1u << b, b);
}

int EqGraph::addBand (float freq, float gainDb, int type, float q)
{
    proc.historyTouch();
    for (int b = 0; b < numBands; ++b)
    {
        if (bandUsed (b)) continue;

        const auto& bp = proc.bandParam[(size_t) b];
        editParam (bp.type, (float) type);
        editParam (bp.freq, freq);
        editParam (bp.gain, gainDb);
        editParam (bp.q, q);
        editParam (bp.dyn, 0.f);
        editParam (bp.on, 1.f);
        editParam (bp.used, 1.f);

        setSelection (1u << b, b);
        return b;
    }
    return -1;   // las 24 bandas están en uso
}

void EqGraph::deleteBand (int b)
{
    proc.historyTouch();
    editParam (proc.bandParam[(size_t) b].used, 0.f);
    setSelection (selMask & ~(1u << b), -1);
}

void EqGraph::deleteSelected()
{
    proc.historyTouch();
    for (int b = 0; b < numBands; ++b)
        if (isSel (b))
            editParam (proc.bandParam[(size_t) b].used, 0.f);
    setSelection (0u, -1);
}

//==============================================================================
//  EqGraph: vista (zoom y desplazamiento)
//==============================================================================
void EqGraph::clampView (View& v) const
{
    const double sr = proc.currentSr.load();
    const double minLo = std::log (10.0), maxHi = std::log (juce::jmin (24000.0, sr * 0.5));
    const double span = juce::jlimit (std::log (2.0), maxHi - minLo, v.hi - v.lo);

    double lo = v.lo;
    if (lo < minLo) lo = minLo;
    if (lo + span > maxHi) lo = maxHi - span;
    v.lo = lo;
    v.hi = lo + span;
    v.half = juce::jlimit (1.5, 36.0, v.half);
    v.center = juce::jlimit (-30.0, 30.0, v.center);
}

void EqGraph::setRange (int db)
{
    tgt.half = db;
    tgt.center = 0.0;
    clampView (tgt);
    animating = true;
    startTimerHz (60);
}

void EqGraph::resetView()
{
    const double sr = proc.currentSr.load();
    tgt = { std::log (20.0), std::log (juce::jmin (20000.0, sr * 0.49)), 0.0, 18.0 };
    clampView (tgt);
    animating = true;
    startTimerHz (60);
}

void EqGraph::zoomFreq (float x, double factor)
{
    auto r = plot();
    const double t = juce::jlimit (0.0, 1.0, (double) ((x - r.getX()) / r.getWidth()));
    const double f0 = tgt.lo + t * (tgt.hi - tgt.lo);
    const double span = (tgt.hi - tgt.lo) * factor;
    tgt.lo = f0 - t * span;
    tgt.hi = tgt.lo + span;
    clampView (tgt);
    animating = true;
    startTimerHz (60);
}

void EqGraph::zoomGain (float y, double factor)
{
    auto r = plot();
    const double t = juce::jlimit (0.0, 1.0, (double) ((r.getBottom() - y) / r.getHeight()));
    const double g0 = tgt.center - tgt.half + t * 2.0 * tgt.half;
    tgt.half = juce::jlimit (1.5, 36.0, tgt.half * factor);
    tgt.center = g0 - (t - 0.5) * 2.0 * tgt.half;
    clampView (tgt);
    animating = true;
    startTimerHz (60);
}

void EqGraph::ensureVisible (float freq)
{
    const double lf = std::log ((double) freq);
    if (lf >= tgt.lo && lf <= tgt.hi)
        return;

    const double span = tgt.hi - tgt.lo;
    tgt.lo = lf - span * 0.5;
    tgt.hi = tgt.lo + span;
    clampView (tgt);
    animating = true;
    startTimerHz (60);
}

bool EqGraph::stepView()
{
    if (! animating)
        return false;

    const double k = 0.35;
    cur.lo     += (tgt.lo - cur.lo) * k;
    cur.hi     += (tgt.hi - cur.hi) * k;
    cur.center += (tgt.center - cur.center) * k;
    cur.half   += (tgt.half - cur.half) * k;

    auto near = [] (double a, double b, double eps) { return std::abs (a - b) < eps; };
    if (near (cur.lo, tgt.lo, 1e-4) && near (cur.hi, tgt.hi, 1e-4) && near (cur.center, tgt.center, 1e-3) && near (cur.half, tgt.half, 1e-3))
    {
        cur = tgt;
        animating = false;
        startTimerHz (30);
    }
    specDirty = true;
    curveDirty = true;
    return true;
}

//==============================================================================
//  EqGraph: temporizador, espectro y resonancias
//==============================================================================
juce::uint32 EqGraph::paramHash() const
{
    juce::uint32 h = 2166136261u;
    auto mix = [&h] (float v)
    {
        juce::uint32 u;
        std::memcpy (&u, &v, sizeof (u));
        h = (h ^ u) * 16777619u;
    };

    for (int b = 0; b < numBands; ++b)
    {
        const auto& p = proc.pBand[(size_t) b];
        mix (p.used->load());  mix (p.on->load());  mix (p.type->load());  mix (p.freq->load());
        mix (p.gain->load());  mix (p.q->load());   mix (p.dyn->load());
        mix (proc.dynShown[(size_t) b].load (std::memory_order_relaxed));
    }
    mix (proc.pHpfOn->load());  mix (proc.pHpfFreq->load());  mix (proc.pHpfSlope->load());
    mix (proc.pLpfOn->load());  mix (proc.pLpfFreq->load());  mix (proc.pLpfSlope->load());
    mix (proc.pBypass->load());
    mix ((float) selMask);
    return h;
}

void EqGraph::timerCallback()
{
    bool need = stepView();

    refreshSelection();

    if (! paused && (showSpectrum || showRes))
    {
        if (updateSpectrum())
        {
            if (showRes) findResonances();
            need = true;
        }
    }

    if (specDirty)
    {
        if (showSpectrum) { rebuildSpectrum(); need = true; }
        specDirty = false;
    }

    const auto h = paramHash();
    if (h != lastHash)
    {
        lastHash = h;
        curveDirty = true;
        need = true;
    }

    if (need)
        repaint();
}

bool EqGraph::updateSpectrum()
{
    const int mask = ParaEQProcessor::scopeSize - 1;
    const int end = proc.scopePos.load (std::memory_order_acquire);

    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = proc.scope[(size_t) ((end - fftSize + i) & mask)].load (std::memory_order_relaxed);
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    float sum = 0.f;
    for (int k = 0; k < fftSize / 2; ++k)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[(size_t) k] / ((float) fftSize * 0.25f), -120.f);
        auto& s = spec[(size_t) k];
        s = db > s ? db : s * 0.85f + db * 0.15f;                          // subida rápida, caída suave
        specSlow[(size_t) k] = specSlow[(size_t) k] * 0.97f + db * 0.03f;   // promedio lento (~1 s)
        sum += s;
    }

    const bool changed = std::abs (sum - lastSpecSum) > 0.05f;
    lastSpecSum = sum;
    if (changed) specDirty = true;
    return changed;
}

void EqGraph::rebuildSpectrum()
{
    specFill.clear();
    specLine.clear();

    auto pl = plot();
    const double sr = proc.currentSr.load();
    const float binHz = (float) (sr / fftSize);
    bool started = false;

    for (float x = pl.getX(); x <= pl.getRight(); x += 2.f)
    {
        const float bLo = xToFreq (x - 1.f) / binHz, bHi = xToFreq (x + 1.f) / binHz;
        float db;
        if (bHi - bLo < 1.f)
        {
            const float bc = 0.5f * (bLo + bHi);
            const int k0 = juce::jlimit (0, fftSize / 2 - 2, (int) bc);
            const float fr = bc - (float) k0;
            db = spec[(size_t) k0] * (1.f - fr) + spec[(size_t) k0 + 1] * fr;
        }
        else
        {
            const int k0 = juce::jlimit (1, fftSize / 2 - 1, (int) bLo), k1 = juce::jlimit (1, fftSize / 2 - 1, (int) bHi);
            db = -120.f;
            for (int k = k0; k <= k1; ++k) db = juce::jmax (db, spec[(size_t) k]);
        }
        const float y = juce::jmap (juce::jlimit (-90.f, 0.f, db), -90.f, 0.f, pl.getBottom(), pl.getY());
        if (! started) { specLine.startNewSubPath (x, y); started = true; }
        else           specLine.lineTo (x, y);
    }

    specFill = specLine;
    specFill.lineTo (pl.getRight(), pl.getBottom());
    specFill.lineTo (pl.getX(), pl.getBottom());
    specFill.closeSubPath();
}

// Detecta picos estrechos que sobresalen del nivel de su entorno (frecuencias molestas)
void EqGraph::findResonances()
{
    res.clear();
    const double sr = proc.currentSr.load();
    const float binHz = (float) (sr / fftSize);
    const int kMin = juce::jmax (6, (int) (120.f / binHz));
    const int kMax = juce::jmin (fftSize / 2 - 8, (int) (12000.f / binHz));

    std::vector<Resonance> cand;
    for (int k = kMin; k <= kMax; ++k)
    {
        const float v = specSlow[(size_t) k];
        if (v < -75.f || v < specSlow[(size_t) k - 1] || v < specSlow[(size_t) k + 1])
            continue;

        const int w = juce::jmax (5, k / 5);
        float sum = 0.f;
        int cnt = 0;
        for (int j = k - w; j <= k + w; ++j)
            if (std::abs (j - k) > 1) { sum += specSlow[(size_t) j]; ++cnt; }

        const float excess = v - sum / (float) cnt;
        if (excess > 6.f)
            cand.push_back ({ (float) k * binHz, excess });
    }

    std::sort (cand.begin(), cand.end(), [] (const Resonance& a, const Resonance& b) { return a.excess > b.excess; });
    for (auto& c : cand)
    {
        bool near = false;
        for (auto& r : res)
            if (std::abs (std::log2 (c.freq / r.freq)) < 0.33f) { near = true; break; }
        if (! near) res.push_back (c);
        if (res.size() >= 6) break;
    }
}

//==============================================================================
//  EqGraph: dibujo
//==============================================================================
void EqGraph::resized()
{
    gridImg = {};
    curveDirty = specDirty = true;
}

void EqGraph::ensureGrid()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    if (gridImg.isValid() && gridImg.getWidth() == w && gridImg.getHeight() == h
        && gridView.lo == cur.lo && gridView.hi == cur.hi && gridView.center == cur.center && gridView.half == cur.half)
        return;

    gridView = cur;
    gridImg = juce::Image (juce::Image::RGB, w, h, false);
    juce::Graphics g (gridImg);

    g.fillAll (Cyber::bg);
    auto area = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (2.f);
    auto frame = Cyber::chamfer (area, 12.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff170e2a), 0.f, area.getY(),
                                             juce::Colour (0xff07040e), 0.f, area.getBottom(), false));
    g.fillPath (frame);

    // sombra interior (relieve hundido)
    g.saveState();
    g.reduceClipRegion (frame);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.strokePath (frame, juce::PathStrokeType (16.f));
    g.restoreState();

    auto pl = plot();

    // ---- líneas de dB
    g.setFont (Cyber::font (11.f));
    const double span = 2.0 * cur.half;
    static const double steps[] = { 0.5, 1.0, 2.0, 3.0, 5.0, 6.0, 10.0, 12.0 };
    double step = 12.0;
    for (double s : steps)
        if (s >= span / 6.0) { step = s; break; }

    for (double k = std::ceil ((cur.center - cur.half) / step); k * step <= cur.center + cur.half + 1e-9; k += 1.0)
    {
        const float v = (float) (k * step);
        const float y = gainToY (v);
        g.setColour (std::abs (v) < 1e-6f ? Cyber::cyan.withAlpha (0.5f) : Cyber::violet.withAlpha (0.16f));
        g.drawHorizontalLine ((int) y, pl.getX(), pl.getRight());
        g.setColour (Cyber::violet.brighter (0.5f));
        g.drawText ((v > 0.f ? "+" : "") + juce::String (v, step < 1.0 ? 1 : 0), 0, (int) y - 7, 36, 14, juce::Justification::centredRight);
    }

    // ---- líneas de frecuencia
    static const float cand[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900,
                                  1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000 };
    auto isMajor = [] (float f)
    {
        for (float m : { 10.f, 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
            if (f == m) return true;
        return false;
    };

    std::vector<float> labelX;
    auto tryLabel = [&] (float f, float x)
    {
        for (float lx : labelX)
            if (std::abs (lx - x) < 44.f) return;
        labelX.push_back (x);
        g.setColour (Cyber::violet.brighter (0.5f));
        g.drawText (f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f),
                    (int) x - 18, (int) pl.getBottom() + 1, 36, 14, juce::Justification::centred);
    };

    for (int pass = 0; pass < 2; ++pass)
        for (float f : cand)
        {
            const float x = freqToX (f);
            if (x < pl.getX() - 0.5f || x > pl.getRight() + 0.5f) continue;
            const bool major = isMajor (f);

            if (pass == 0)
            {
                g.setColour (Cyber::violet.withAlpha (major ? 0.22f : 0.09f));
                g.drawVerticalLine ((int) x, pl.getY(), pl.getBottom());
                if (major) tryLabel (f, x);
            }
            else if (! major)
            {
                tryLabel (f, x);
            }
        }

    // ---- borde neón
    g.setColour (Cyber::cyan.withAlpha (0.16f));
    g.strokePath (frame, juce::PathStrokeType (6.f));
    g.setColour (Cyber::cyan.withAlpha (0.65f));
    g.strokePath (frame, juce::PathStrokeType (1.3f));
}

void EqGraph::rebuildCurve()
{
    const double sr = proc.currentSr.load();
    auto pl = plot();

    EqMath::Coef hp[EqMath::maxStages], lp[EqMath::maxStages];
    int nh = 0, nl = 0;
    if (proc.pHpfOn->load() > 0.5f) nh = EqMath::designCut (true,  sr, proc.pHpfFreq->load(), (int) std::lround (proc.pHpfSlope->load()), hp);
    if (proc.pLpfOn->load() > 0.5f) nl = EqMath::designCut (false, sr, proc.pLpfFreq->load(), (int) std::lround (proc.pLpfSlope->load()), lp);

    EqMath::Coef bc[numBands];
    bool act[numBands] {};
    for (int b = 0; b < numBands; ++b)
    {
        const auto bd = band (b);
        act[b] = bd.used && bd.on;
        if (! act[b]) continue;

        float gain = bd.gain;
        if (bd.type != EqMath::Notch && bd.dyn)
            gain += proc.dynShown[(size_t) b].load (std::memory_order_relaxed);   // la curva sigue a la dinámica en vivo
        bc[b] = EqMath::designBand (bd.type, sr, bd.freq, gain, bd.q);
    }

    auto dbToY = [this] (double mag)
    {
        const double db = 20.0 * std::log10 (std::max (mag, 1e-9));
        return gainToY ((float) juce::jlimit (cur.center - cur.half * 1.3, cur.center + cur.half * 1.3, db));
    };

    curvePath.clear();
    selFill.clear();

    const bool selActive = selected >= 0 && act[selected];
    const float y0 = gainToY (0.f);
    if (selActive) selFill.startNewSubPath (pl.getX(), y0);

    const int n = juce::jmax (2, (int) std::ceil (pl.getWidth() / 2.f) + 1);
    float xLast = pl.getX();
    for (int i = 0; i < n; ++i)
    {
        const float x = pl.getX() + (float) i * 2.f;
        xLast = x;
        const double w = 2.0 * EqMath::pi * (double) xToFreq (x) / sr;
        const double cw = std::cos (w), c2 = std::cos (2.0 * w), sw = std::sin (w), s2 = std::sin (2.0 * w);

        double mag = 1.0;
        for (int k = 0; k < nh; ++k) mag *= EqMath::magnitudeTrig (hp[k], cw, c2, sw, s2);
        for (int k = 0; k < nl; ++k) mag *= EqMath::magnitudeTrig (lp[k], cw, c2, sw, s2);
        for (int b = 0; b < numBands; ++b)
            if (act[b]) mag *= EqMath::magnitudeTrig (bc[b], cw, c2, sw, s2);

        const float y = dbToY (mag);
        if (i == 0) curvePath.startNewSubPath (x, y); else curvePath.lineTo (x, y);
        if (selActive) selFill.lineTo (x, dbToY (EqMath::magnitudeTrig (bc[selected], cw, c2, sw, s2)));
    }

    if (selActive)
    {
        selFill.lineTo (xLast, y0);
        selFill.closeSubPath();
    }
}

void EqGraph::paint (juce::Graphics& g)
{
    ensureGrid();
    if (gridImg.isValid())
        g.drawImageAt (gridImg, 0, 0);
    else
        g.fillAll (Cyber::bg);

    if (curveDirty)
    {
        rebuildCurve();
        curveDirty = false;
    }

    auto pl = plot();
    const bool bypassed = proc.pBypass->load() > 0.5f;

    g.saveState();
    g.reduceClipRegion (pl.toNearestInt());

    if (showSpectrum)
    {
        g.setColour (Cyber::magenta.withAlpha (0.16f));
        g.fillPath (specFill);
        g.setColour (Cyber::magenta.withAlpha (0.55f));
        g.strokePath (specLine, juce::PathStrokeType (1.2f));
    }

    if (selected >= 0 && ! bypassed)
    {
        g.setColour (Cyber::band (selected).withAlpha (0.20f));
        g.fillPath (selFill);
    }

    const auto lineCol = bypassed ? juce::Colours::grey : Cyber::cyan;
    g.setColour (lineCol.withAlpha (0.10f));
    g.strokePath (curvePath, juce::PathStrokeType (10.f));
    g.setColour (lineCol.withAlpha (0.25f));
    g.strokePath (curvePath, juce::PathStrokeType (5.f));
    g.setColour (lineCol.brighter (0.5f));
    g.strokePath (curvePath, juce::PathStrokeType (1.8f));

    g.restoreState();

    // ---- resonancias (frecuencias molestas)
    if (showRes)
    {
        const float dash[2] = { 4.f, 4.f };
        g.setFont (Cyber::font (12.f, true));
        for (auto& r : res)
        {
            const float x = freqToX (r.freq);
            if (x < pl.getX() || x > pl.getRight()) continue;

            g.setColour (Cyber::magenta.withAlpha (0.6f));
            g.drawDashedLine (juce::Line<float> (x, pl.getY() + 12.f, x, pl.getBottom()), dash, 2, 1.f);

            juce::Path tri;
            tri.addTriangle (x - 7.f, pl.getY(), x + 7.f, pl.getY(), x, pl.getY() + 12.f);
            g.setColour (Cyber::magenta.withAlpha (0.3f));
            g.strokePath (tri, juce::PathStrokeType (5.f));
            g.setColour (Cyber::magenta);
            g.fillPath (tri);

            const float tx = (x > pl.getRight() - 150.f) ? x - 148.f : x + 8.f;
            g.setColour (juce::Colours::white);
            g.drawText (fmtHz (r.freq) + " +" + juce::String ((int) std::lround (r.excess)) + "dB",
                        (int) tx, (int) pl.getY() + 1, 140, 14, juce::Justification::centredLeft);
        }
    }

    // ---- puntos de las bandas (esferas con relieve)
    for (int b = 0; b < numBands; ++b)
    {
        const auto bd = band (b);
        if (! bd.used) continue;

        const auto c = nodePos (b);
        if (! nodeVisible (c)) continue;

        const bool active = bd.on;
        const auto r = juce::Rectangle<float> (18.f, 18.f).withCentre (c);
        const auto col = Cyber::band (b);

        if (active)
        {
            g.setColour (col.withAlpha (0.25f));
            g.fillEllipse (r.expanded (5.f));
            g.setGradientFill (juce::ColourGradient (col.brighter (0.8f), c.x - 3.f, c.y - 4.f, col.darker (0.5f), c.x + 7.f, c.y + 8.f, true));
            g.fillEllipse (r);
        }
        else
        {
            g.setColour (Cyber::bg);
            g.fillEllipse (r);
            g.setColour (col.withAlpha (0.6f));
            g.drawEllipse (r, 2.f);
        }

        if (bd.type != EqMath::Notch && bd.dyn)
        {
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.drawEllipse (r.expanded (3.f), 1.f);

            const float off = proc.dynShown[(size_t) b].load (std::memory_order_relaxed);
            if (std::abs (off) > 0.1f)
            {
                const float gy = juce::jlimit (pl.getY(), pl.getBottom(), gainToY (bd.gain + off));
                g.setColour (col.withAlpha (0.8f));
                g.drawLine (c.x, c.y, c.x, gy, 1.5f);
                g.drawEllipse (juce::Rectangle<float> (10.f, 10.f).withCentre ({ c.x, gy }), 1.5f);
            }
        }

        g.setColour (active ? juce::Colours::black : col);
        g.setFont (Cyber::font (11.5f, true));
        g.drawText (juce::String (b + 1), r.toNearestInt(), juce::Justification::centred);

        if (isSel (b))
        {
            g.setColour (juce::Colours::white.withAlpha (b == selected ? 1.f : 0.6f));
            g.drawEllipse (r.expanded (5.f), b == selected ? 2.f : 1.4f);
        }
    }

    // ---- rectángulo de selección
    if (drag == Drag::Marquee && ! marquee.isEmpty())
    {
        g.setColour (Cyber::cyan.withAlpha (0.12f));
        g.fillRect (marquee);
        g.setColour (Cyber::cyan.withAlpha (0.85f));
        g.drawRect (marquee, 1.f);
    }

    // ---- textos
    g.setFont (Cyber::font (12.5f, true));
    if (selected >= 0 && bandUsed (selected))
    {
        auto txt = [this] (juce::RangedAudioParameter* p) { return p->getCurrentValueAsText(); };
        const auto& bp = proc.bandParam[(size_t) selected];
        g.setColour (Cyber::band (selected));
        const int n = selectedCount();
        g.drawText ((n > 1 ? juce::String (n) + " BANDAS  //  PRINCIPAL " : juce::String ("BANDA ")) + juce::String (selected + 1)
                    + "  //  " + txt (bp.freq) + "  //  " + txt (bp.gain) + "  //  Q " + txt (bp.q),
                    (int) pl.getX() + 8, (int) pl.getBottom() - 20, 520, 16, juce::Justification::centredLeft);
    }

    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText (juce::String ("BANDAS ") + juce::String (numUsed()) + "/" + juce::String (numBands),
                (int) pl.getRight() - 120, (int) pl.getBottom() - 20, 112, 16, juce::Justification::centredRight);

    if (paused)
    {
        g.setColour (Cyber::yellow);
        g.drawText (Cyber::u8 ("ANALIZADOR EN PAUSA"), (int) pl.getRight() - 230, (int) pl.getY() + 4, 222, 16, juce::Justification::centredRight);
    }

    if (numUsed() == 0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.setFont (Cyber::font (20.f, true));
        g.drawText (Cyber::u8 ("Doble clic para añadir una banda"), pl.toNearestInt().withTrimmedBottom ((int) pl.getHeight() / 3), juce::Justification::centred);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.setFont (Cyber::font (13.f));
        g.drawText (Cyber::u8 ("Arrastra para seleccionar varias · Rueda: zoom · Alt + arrastrar: mover la vista"),
                    pl.toNearestInt().withTrimmedTop ((int) pl.getHeight() / 2 - 10).withTrimmedBottom ((int) pl.getHeight() / 3), juce::Justification::centred);
    }
}

//==============================================================================
//  EqGraph: ratón
//==============================================================================
void EqGraph::showMenu (int b)
{
    const juce::uint32 mask = isSel (b) ? selMask : (1u << b);
    const auto bd = band (b);
    int count = 0;
    for (int i = 0; i < numBands; ++i) if ((mask >> i) & 1u) ++count;

    juce::PopupMenu m;
    m.addItem (1, count > 1 ? "Eliminar bandas seleccionadas" : "Eliminar banda");
    m.addItem (2, bd.on ? "Desactivar" : "Activar");
    m.addItem (3, bd.dyn ? Cyber::u8 ("Quitar dinámica") : Cyber::u8 ("Activar dinámica"));

    juce::Component::SafePointer<EqGraph> self (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [self, mask, bd] (int r)
    {
        if (self == nullptr || r == 0) return;
        self->proc.historyTouch();

        for (int i = 0; i < numBands; ++i)
        {
            if (! ((mask >> i) & 1u)) continue;
            const auto& bp = self->proc.bandParam[(size_t) i];
            if (r == 1)      self->editParam (bp.used, 0.f);
            else if (r == 2) self->editParam (bp.on,  bd.on  ? 0.f : 1.f);
            else if (r == 3) self->editParam (bp.dyn, bd.dyn ? 0.f : 1.f);
        }
        if (r == 1)
            self->setSelection (self->selMask & ~mask, -1);
    });
}

void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    proc.historyTouch();
    drag = Drag::None;
    dragBand = -1;

    const int b = hitTest (e.position);

    // mover la vista: Alt + arrastrar en vacío, o botón central
    if (e.mods.isMiddleButtonDown() || (e.mods.isAltDown() && b < 0))
    {
        drag = Drag::Pan;
        panStart = e.position;
        panStartView = tgt;
        return;
    }

    // clic en el triángulo de una resonancia: crea una banda que la reduce
    if (showRes)
        for (auto& r : res)
            if (std::abs (e.position.x - freqToX (r.freq)) < 10.f && e.position.y < plot().getY() + 24.f)
            {
                addBand (r.freq, -juce::jlimit (3.f, 12.f, r.excess * 0.7f), EqMath::Bell, 6.f);
                return;
            }

    if (b < 0)
    {
        // clic en vacío: selección rectangular
        drag = Drag::Marquee;
        marqueeStart = e.position;
        marquee = {};
        marqueeBase = (e.mods.isShiftDown() || e.mods.isCommandDown()) ? selMask : 0u;
        if (marqueeBase == 0u)
            setSelection (0u, -1);
        return;
    }

    if (e.mods.isPopupMenu())
    {
        if (! isSel (b)) setSelection (1u << b, b);
        showMenu (b);
        return;
    }

    if (e.mods.isShiftDown() || e.mods.isCommandDown())
    {
        const auto m = selMask ^ (1u << b);
        setSelection (m, (m >> b) & 1u ? b : -1);
        if (! isSel (b)) return;
    }
    else if (! isSel (b))
    {
        setSelection (1u << b, b);
    }
    else if (selected != b)
    {
        setSelection (selMask, b);
    }

    // preparar el arrastre del grupo
    drag = Drag::Node;
    dragBand = b;
    gestures.clear();
    for (int i = 0; i < numBands; ++i)
    {
        if (! isSel (i)) continue;
        const auto bd = band (i);
        startF[i] = bd.freq;
        startG[i] = bd.gain;
        const auto& bp = proc.bandParam[(size_t) i];
        bp.freq->beginChangeGesture();
        bp.gain->beginChangeGesture();
        gestures.push_back (bp.freq);
        gestures.push_back (bp.gain);
    }

    anchorMouse = e.position;
    anchorNode  = nodePos (b);
    lastFine    = e.mods.isAltDown();
}

void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    auto pl = plot();

    if (drag == Drag::Pan)
    {
        const auto d = e.position - panStart;
        const double perPx = (panStartView.hi - panStartView.lo) / (double) pl.getWidth();
        const double dbPerPx = 2.0 * panStartView.half / (double) pl.getHeight();
        tgt = panStartView;
        tgt.lo -= (double) d.x * perPx;
        tgt.hi -= (double) d.x * perPx;
        tgt.center += (double) d.y * dbPerPx;
        clampView (tgt);
        cur = tgt;
        specDirty = curveDirty = true;
        repaint();
        return;
    }

    if (drag == Drag::Marquee)
    {
        marquee = juce::Rectangle<float> (marqueeStart, e.position);
        juce::uint32 m = marqueeBase;
        for (int b = 0; b < numBands; ++b)
            if (bandUsed (b) && marquee.contains (nodePos (b)))
                m |= (1u << b);
        setSelection (m, -1);
        repaint();
        return;
    }

    if (drag != Drag::Node || dragBand < 0)
        return;

    // Alt = ajuste fino (10 veces más lento)
    const bool fine = e.mods.isAltDown();
    if (fine != lastFine)
    {
        anchorMouse = e.position;
        anchorNode  = nodePos (dragBand);
        lastFine    = fine;
    }
    const auto target = anchorNode + (e.position - anchorMouse) * (fine ? 0.1f : 1.0f);

    const double dOct = std::log2 ((double) xToFreq (target.x) / (double) startF[dragBand]);
    const float dG = yToGain (target.y) - startG[dragBand];

    for (int i = 0; i < numBands; ++i)
    {
        if (! isSel (i)) continue;
        const auto& bp = proc.bandParam[(size_t) i];

        if (proc.pBand[(size_t) i].on->load() < 0.5f)       // al mover una banda apagada, se enciende
            setNorm (bp.on, 1.f);

        setNorm (bp.freq, (float) juce::jlimit (20.0, 20000.0, (double) startF[i] * std::pow (2.0, dOct)));
        if ((int) std::lround (proc.pBand[(size_t) i].type->load()) != EqMath::Notch)
            setNorm (bp.gain, juce::jlimit (-24.f, 24.f, startG[i] + dG));
    }
}

void EqGraph::mouseUp (const juce::MouseEvent&)
{
    for (auto* p : gestures)
        p->endChangeGesture();
    gestures.clear();

    if (drag == Drag::Marquee)
    {
        marquee = {};
        repaint();
    }
    drag = Drag::None;
    dragBand = -1;
}

void EqGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    proc.historyTouch();
    const int b = hitTest (e.position);
    if (b >= 0)
    {
        editParam (proc.bandParam[(size_t) b].gain, 0.f);
        return;
    }

    // doble clic en un espacio vacío: nueva banda
    if (plot().contains (e.position))
        addBand (xToFreq (e.position.x), yToGain (e.position.y), EqMath::Bell, 1.f);
}

void EqGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    proc.historyTouch();
    const int b = hitTest (e.position);

    if (b >= 0)
    {
        // sobre un punto: Q de la banda (o de todas las seleccionadas)
        const juce::uint32 mask = isSel (b) ? selMask : (1u << b);
        const float k = std::exp (w.deltaY * (e.mods.isAltDown() ? 0.15f : 1.5f));
        for (int i = 0; i < numBands; ++i)
            if ((mask >> i) & 1u)
                editParam (proc.bandParam[(size_t) i].q, proc.pBand[(size_t) i].q->load() * k);
        return;
    }

    if (std::abs (w.deltaX) > std::abs (w.deltaY))
    {
        const double span = tgt.hi - tgt.lo;
        tgt.lo -= (double) w.deltaX * span * 0.5;
        tgt.hi = tgt.lo + span;
        clampView (tgt);
        animating = true;
        startTimerHz (60);
    }
    else if (e.mods.isAltDown() || e.mods.isShiftDown())
    {
        zoomGain (e.position.y, std::exp (-(double) w.deltaY * 1.2));
    }
    else
    {
        zoomFreq (e.position.x, std::exp (-(double) w.deltaY * 1.2));
    }
}

void EqGraph::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    if (scaleFactor > 0.f)
        zoomFreq (e.position.x, 1.0 / (double) scaleFactor);
}

//==============================================================================
//  ParaEQEditor
//==============================================================================
ParaEQEditor::ParaEQEditor (ParaEQProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      graph (p, [this] (int b) { selectBand (b); })
{
    setLookAndFeel (&laf);
    setWantsKeyboardFocus (true);
    proc.uiOpen.store (true);

    // ---- estructura
    addAndMakeVisible (graph);
    addAndMakeVisible (topRegion);
    addAndMakeVisible (bottomRegion);
    addAndMakeVisible (leftRegion);
    addAndMakeVisible (rightRegion);
    leftRegion.addAndMakeVisible (meterIn);
    rightRegion.addAndMakeVisible (meterOut);
    bottomRegion.addAndMakeVisible (filtersPanel);
    bottomRegion.addAndMakeVisible (bandPanel);

    // ---- estilo de botones
    auto toggle = [] (CyberButton& b, const juce::String& text, juce::Colour neon)
    {
        b.setButtonText (text);
        b.setClickingTogglesState (true);
        b.setNeon (neon);
    };
    auto action = [] (CyberButton& b, const juce::String& text, juce::Colour neon)
    {
        b.setButtonText (text);
        b.setNeon (neon);
    };

    toggle (bypassBtn, "BYPASS", Cyber::orange);
    toggle (polBtn, Cyber::u8 ("Ø POL"), Cyber::magenta);
    toggle (analyzerBtn, "ANALIZADOR", Cyber::cyan);
    toggle (resBtn, "RESONANCIAS", Cyber::magenta);
    toggle (pauseBtn, "PAUSA", Cyber::yellow);
    toggle (hpfOn, "ON", Cyber::green);
    toggle (lpfOn, "ON", Cyber::green);
    toggle (bandOn, "ACTIVA", Cyber::green);
    toggle (dynBtn, Cyber::u8 ("DINÁMICA"), Cyber::yellow);
    action (infoBtn, "INFO", Cyber::yellow);
    action (presetBtn, "PRESETS", Cyber::cyan);
    action (saveBtn, "GUARDAR", Cyber::green);
    action (clearBtn, "BORRAR TODO", Cyber::magenta);
    action (centerBtn, "CENTRAR VISTA", Cyber::cyan);
    action (addBtn, "+ BANDA", Cyber::cyan);
    action (delBtn, "ELIMINAR", Cyber::magenta);

    undoBtn.setNeon (Cyber::yellow);  undoBtn.setArrow (0.5f);
    redoBtn.setNeon (Cyber::yellow);  redoBtn.setArrow (0.f);
    analyzerBtn.setToggleState (true, juce::dontSendNotification);

    // ---- barra superior
    for (auto* c : std::initializer_list<juce::Component*> { &analyzerBtn, &resBtn, &rangeBox, &infoBtn, &bypassBtn, &polBtn,
                                                              &lblIn, &inSl, &lblOut, &outSl, &undoBtn, &redoBtn,
                                                              &presetBtn, &saveBtn, &clearBtn, &centerBtn, &pauseBtn })
        topRegion.addAndMakeVisible (*c);

    label (lblIn, "IN");  label (lblOut, "OUT");
    for (auto* s : { &inSl, &outSl })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 78, 20);
        s->setColour (juce::Slider::trackColourId, Cyber::cyan);
    }

    rangeBox.addItem (Cyber::u8 ("Rango ±6 dB"), 1);
    rangeBox.addItem (Cyber::u8 ("Rango ±12 dB"), 2);
    rangeBox.addItem (Cyber::u8 ("Rango ±18 dB"), 3);
    rangeBox.addItem (Cyber::u8 ("Rango ±24 dB"), 4);
    rangeBox.setSelectedId (3, juce::dontSendNotification);
    rangeBox.onChange = [this] { graph.setRange (rangeBox.getSelectedId() * 6); };

    analyzerBtn.onClick = [this] { graph.setSpectrum (analyzerBtn.getToggleState()); };
    resBtn.onClick      = [this] { graph.setResonances (resBtn.getToggleState()); };
    pauseBtn.onClick    = [this] { graph.setPaused (pauseBtn.getToggleState()); };
    centerBtn.onClick   = [this] { graph.resetView(); rangeBox.setSelectedId (3, juce::dontSendNotification); };
    infoBtn.onClick     = [this] { info.setVisible (true); info.toFront (true); };

    undoBtn.onClick = [this] { proc.undo(); graph.refreshSelection(); };
    redoBtn.onClick = [this] { proc.redo(); graph.refreshSelection(); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    saveBtn.onClick   = [this] { askSavePreset(); };
    clearBtn.onClick  = [this]
    {
        runProgrammatic ([this] { Presets::clearEq (proc.apvts, true); });
        setPresetName ({});
    };

    // ---- panel de filtros de corte
    for (auto* c : std::initializer_list<juce::Component*> { &lblHpf, &hpfOn, &hpfFreq, &hpfSlope, &lblLpf, &lpfOn, &lpfFreq, &lpfSlope })
        filtersPanel.addAndMakeVisible (*c);
    label (lblHpf, "HPF");  label (lblLpf, "LPF");
    knob (hpfFreq);  knob (lpfFreq);
    for (auto* c : { &hpfSlope, &lpfSlope })
        c->addItemList ({ "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }, 1);

    // ---- panel de banda
    for (auto* c : std::initializer_list<juce::Component*> { &bType, &bandOn, &dynBtn, &addBtn, &delBtn,
                                                              &bFreq, &bGain, &bQ, &dRange, &dThr, &dAtk, &dRel,
                                                              &lblFreq, &lblGain, &lblQ, &lblRange, &lblThr, &lblAtk, &lblRel })
        bandPanel.addAndMakeVisible (*c);

    bType.addItemList ({ "Campana", "Shelf graves", "Shelf agudos", "Notch" }, 1);
    for (auto* s : { &bFreq, &bGain, &bQ, &dRange, &dThr, &dAtk, &dRel })
        knob (*s);
    label (lblFreq, "FRECUENCIA");  label (lblGain, "GANANCIA");  label (lblQ, "Q");
    label (lblRange, "RANGO");      label (lblThr, "UMBRAL");     label (lblAtk, "ATAQUE");  label (lblRel, "RELEASE");

    addBtn.onClick = [this] { graph.addBand (1000.f, 0.f, EqMath::Bell, 1.f); };
    delBtn.onClick = [this] { graph.deleteSelected(); };

    // ---- títulos y textos de los paneles
    topRegion.paintOver = [] (juce::Graphics& g)
    {
        g.setFont (Cyber::font (30.f, true));
        g.setColour (Cyber::magenta.withAlpha (0.35f));
        g.drawText ("SemeParaEQ", juce::Rectangle<int> (2, 1, 230, 36), juce::Justification::centredLeft);
        g.setColour (Cyber::cyan);
        g.drawText ("SemeParaEQ", juce::Rectangle<int> (0, 0, 230, 36), juce::Justification::centredLeft);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.setFont (Cyber::font (10.f, true));
        g.drawText (Cyber::u8 ("ECUALIZADOR PARAMÉTRICO"), juce::Rectangle<int> (2, 34, 230, 14), juce::Justification::centredLeft);
    };

    filtersPanel.paintOver = [] (juce::Graphics& g)
    {
        g.setColour (Cyber::magenta);
        g.setFont (Cyber::font (13.f, true));
        g.drawText ("// FILTROS DE CORTE", 20, 14, 250, 18, juce::Justification::centredLeft);
    };

    bandPanel.paintOver = [this] (juce::Graphics& g)
    {
        g.setColour (Cyber::magenta);
        g.setFont (Cyber::font (13.f, true));

        juce::String title = Cyber::u8 ("// SIN BANDA SELECCIONADA");
        if (selected >= 0)
        {
            const int n = graph.selectedCount();
            title = "// BANDA " + juce::String (selected + 1) + "  -  " + bType.getText().toUpperCase();
            if (n > 1) title += "   (" + juce::String (n) + Cyber::u8 (" bandas se mueven juntas)");
        }
        g.drawText (title, 20, 14, 560, 18, juce::Justification::centredLeft);

        g.setFont (Cyber::font (11.f, true));
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawText ("FILTRO", capFilter, juce::Justification::centred);
        g.drawText (Cyber::u8 ("DINÁMICA"), capDyn, juce::Justification::centred);
    };

    // ---- asas para minimizar zonas
    static const float openDir[4] = { 0.75f, 0.25f, 0.5f, 0.f };
    for (int i = 0; i < 4; ++i)
    {
        handle[i].setNeon (Cyber::cyan);
        handle[i].setArrow (openDir[i]);
        handle[i].onClick = [this, i] { togglePanel (i); };
        addAndMakeVisible (handle[i]);
    }

    // ---- enlace a Instagram
    link.setFont (juce::Font (Cyber::font (11.5f)), false, juce::Justification::centredRight);
    link.setColour (juce::HyperlinkButton::textColourId, Cyber::cyan.withAlpha (0.8f));
    addAndMakeVisible (link);

    animTimer.fn = [this] { stepPanels(); };
    addChildComponent (info);
    info.onClose = [this] { info.setVisible (false); };

    // ---- conexión con los parámetros
    auto& a = proc.apvts;
    aIn = std::make_unique<SA> (a, "inGain", inSl);
    aOut = std::make_unique<SA> (a, "outGain", outSl);
    aBypass = std::make_unique<BA> (a, "bypass", bypassBtn);
    aPol = std::make_unique<BA> (a, "polarity", polBtn);
    aHpfOn = std::make_unique<BA> (a, "hpfOn", hpfOn);
    aHpfFreq = std::make_unique<SA> (a, "hpfFreq", hpfFreq);
    aHpfSlope = std::make_unique<CA> (a, "hpfSlope", hpfSlope);
    aLpfOn = std::make_unique<BA> (a, "lpfOn", lpfOn);
    aLpfFreq = std::make_unique<SA> (a, "lpfFreq", lpfFreq);
    aLpfSlope = std::make_unique<CA> (a, "lpfSlope", lpfSlope);

    selectBand (-1);
    setResizable (true, true);
    setResizeLimits (1260, 460, 2600, 1800);
    setSize (1260, 840);
    startTimerHz (30);
}

ParaEQEditor::~ParaEQEditor()
{
    animTimer.stopTimer();
    proc.uiOpen.store (false);
    setLookAndFeel (nullptr);
}

void ParaEQEditor::knob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 92, 18);
    s.setColour (juce::Slider::rotarySliderFillColourId, Cyber::cyan);
}

void ParaEQEditor::label (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, Cyber::cyan.withAlpha (0.9f));
    l.setFont (juce::Font (Cyber::font (11.5f, true)));
}

void ParaEQEditor::selectBand (int b)
{
    selected = b;

    aBandOn.reset(); aDyn.reset(); aBType.reset(); aBFreq.reset(); aBGain.reset(); aBQ.reset();
    aDRange.reset(); aDThr.reset(); aDAtk.reset(); aDRel.reset();

    if (b >= 0)
    {
        auto& a = proc.apvts;
        aBandOn = std::make_unique<BA> (a, bandId (b, "on"), bandOn);
        aDyn    = std::make_unique<BA> (a, bandId (b, "dyn"), dynBtn);
        aBType  = std::make_unique<CA> (a, bandId (b, "type"), bType);
        aBFreq  = std::make_unique<SA> (a, bandId (b, "freq"), bFreq);
        aBGain  = std::make_unique<SA> (a, bandId (b, "gain"), bGain);
        aBQ     = std::make_unique<SA> (a, bandId (b, "q"), bQ);
        aDRange = std::make_unique<SA> (a, bandId (b, "dRange"), dRange);
        aDThr   = std::make_unique<SA> (a, bandId (b, "dThr"), dThr);
        aDAtk   = std::make_unique<SA> (a, bandId (b, "dAtk"), dAtk);
        aDRel   = std::make_unique<SA> (a, bandId (b, "dRel"), dRel);

        for (auto* s : { &bFreq, &bGain, &bQ, &dRange, &dThr, &dAtk, &dRel })
            s->setColour (juce::Slider::rotarySliderFillColourId, Cyber::band (b));
    }

    updateEnabled();
    bandPanel.repaint();
}

void ParaEQEditor::updateEnabled()
{
    const bool has = selected >= 0;
    const bool notch = has && (bType.getSelectedId() - 1) == EqMath::Notch;
    const bool dynOn = has && ! notch && dynBtn.getToggleState();

    auto en = [] (juce::Component& c, bool on)
    {
        if (c.isEnabled() != on) c.setEnabled (on);
        c.setAlpha (on ? 1.f : 0.35f);
    };
    en (bandOn, has);  en (bType, has);  en (bFreq, has);  en (bQ, has);  en (delBtn, has);
    en (bGain, has && ! notch);  en (dynBtn, has && ! notch);
    en (dRange, dynOn);  en (dThr, dynOn);  en (dAtk, dynOn);  en (dRel, dynOn);
}

void ParaEQEditor::timerCallback()
{
    auto db = [] (float amp) { return juce::Decibels::gainToDecibels (amp, -100.f); };

    for (size_t c = 0; c < 2; ++c)
    {
        inR[c]  = juce::jmax (std::sqrt (2.f * proc.inMs[c].load()),  inR[c]  * 0.90f);
        outR[c] = juce::jmax (std::sqrt (2.f * proc.outMs[c].load()), outR[c] * 0.90f);
        inP[c]  = juce::jmax (proc.inPk[c].load(),  inP[c]  * 0.97f);
        outP[c] = juce::jmax (proc.outPk[c].load(), outP[c] * 0.97f);
    }
    if (vis[2] > 0.01f) meterIn.setLevels  (db (inR[0]),  db (inR[1]),  db (inP[0]),  db (inP[1]));
    if (vis[3] > 0.01f) meterOut.setLevels (db (outR[0]), db (outR[1]), db (outP[0]), db (outP[1]));

    updateEnabled();

    // Historial de deshacer / rehacer: solo se anotan los cambios hechos por el usuario
    const bool raw = juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown()
                  || dynamic_cast<juce::TextEditor*> (juce::Component::getCurrentlyFocusedComponent()) != nullptr;
    proc.historyTick (raw);

    const bool cu = proc.canUndo(), cr = proc.canRedo();
    undoBtn.setEnabled (cu);  undoBtn.setAlpha (cu ? 1.f : 0.3f);
    redoBtn.setEnabled (cr);  redoBtn.setAlpha (cr ? 1.f : 0.3f);
}

//==============================================================================
//  Maquetación, zonas minimizables y animación
//==============================================================================
void ParaEQEditor::togglePanel (int i)
{
    static const float openDir[4] = { 0.75f, 0.25f, 0.5f, 0.f };

    visTarget[i] = visTarget[i] > 0.5f ? 0.f : 1.f;
    const bool open = visTarget[i] > 0.5f;
    handle[i].setArrow (open ? openDir[i] : std::fmod (openDir[i] + 0.5f, 1.f));
    animTimer.startTimerHz (60);
}

void ParaEQEditor::stepPanels()
{
    bool moving = false;
    for (int i = 0; i < 4; ++i)
    {
        if (vis[i] == visTarget[i]) continue;
        vis[i] += (visTarget[i] - vis[i]) * 0.28f;
        if (std::abs (vis[i] - visTarget[i]) < 0.004f) vis[i] = visTarget[i];
        moving = true;
    }
    layoutMain();
    if (! moving)
        animTimer.stopTimer();
}

void ParaEQEditor::layoutMain()
{
    auto r = getLocalBounds().reduced (12, 10);
    footer = r.removeFromBottom (20);
    link.setBounds (footer.removeFromRight (200));

    const int topH   = (int) std::lround (vis[0] * topFull);
    const int botH   = (int) std::lround (vis[1] * botFull);
    const int leftW  = (int) std::lround (vis[2] * meterFull);
    const int rightW = (int) std::lround (vis[3] * meterFull);

    topRegion.setBounds (r.removeFromTop (topH));
    r.removeFromTop ((int) std::lround (vis[0] * 6.f));
    bottomRegion.setBounds (r.removeFromBottom (botH));
    r.removeFromBottom ((int) std::lround (vis[1] * 10.f));
    leftRegion.setBounds (r.removeFromLeft (leftW));
    r.removeFromLeft ((int) std::lround (vis[2] * 8.f));
    rightRegion.setBounds (r.removeFromRight (rightW));
    r.removeFromRight ((int) std::lround (vis[3] * 8.f));
    graph.setBounds (r);

    topRegion.setVisible (topH > 0);
    bottomRegion.setVisible (botH > 0);
    leftRegion.setVisible (leftW > 0);
    rightRegion.setVisible (rightW > 0);

    meterIn.setBounds (0, 0, meterFull, leftRegion.getHeight());
    meterOut.setBounds (0, 0, meterFull, rightRegion.getHeight());
    if (topH > 0) layoutTop();
    if (botH > 0) layoutBottom();

    const auto gb = graph.getBounds();
    handle[0].setBounds (gb.getCentreX() - 20, gb.getY() + 4, 40, 16);
    handle[1].setBounds (gb.getCentreX() - 20, gb.getBottom() - 20, 40, 16);
    handle[2].setBounds (gb.getX() + 4, gb.getCentreY() - 20, 16, 40);
    handle[3].setBounds (gb.getRight() - 20, gb.getCentreY() - 20, 16, 40);
    for (auto& h : handle) h.toFront (false);
}

void ParaEQEditor::layoutTop()
{
    const int W = topRegion.getWidth();

    // fila 1
    auto r1 = juce::Rectangle<int> (0, 0, W, 50);
    r1.removeFromLeft (236);
    analyzerBtn.setBounds (r1.removeFromLeft (104).reduced (0, 10));  r1.removeFromLeft (6);
    resBtn.setBounds      (r1.removeFromLeft (118).reduced (0, 10));  r1.removeFromLeft (6);
    rangeBox.setBounds    (r1.removeFromLeft (112).reduced (0, 10));  r1.removeFromLeft (6);
    infoBtn.setBounds     (r1.removeFromLeft (60).reduced (0, 10));

    bypassBtn.setBounds (r1.removeFromRight (84).reduced (0, 10));  r1.removeFromRight (6);
    polBtn.setBounds    (r1.removeFromRight (84).reduced (0, 10));  r1.removeFromRight (10);

    auto placeGain = [] (juce::Label& l, juce::Slider& s, juce::Rectangle<int> b)
    {
        l.setBounds (b.removeFromLeft (34));
        s.setBounds (b);
    };
    placeGain (lblOut, outSl, r1.removeFromRight (190));  r1.removeFromRight (8);
    placeGain (lblIn,  inSl,  r1.removeFromRight (190));

    // fila 2
    auto r2 = juce::Rectangle<int> (0, 56, W, 36);
    undoBtn.setBounds (r2.removeFromLeft (36).reduced (0, 2));  r2.removeFromLeft (6);
    redoBtn.setBounds (r2.removeFromLeft (36).reduced (0, 2));  r2.removeFromLeft (14);
    presetBtn.setBounds (r2.removeFromLeft (290).reduced (0, 2));  r2.removeFromLeft (6);
    saveBtn.setBounds   (r2.removeFromLeft (100).reduced (0, 2));  r2.removeFromLeft (14);
    clearBtn.setBounds  (r2.removeFromLeft (120).reduced (0, 2));

    pauseBtn.setBounds  (r2.removeFromRight (90).reduced (0, 2));   r2.removeFromRight (6);
    centerBtn.setBounds (r2.removeFromRight (140).reduced (0, 2));
}

void ParaEQEditor::layoutBottom()
{
    const int W = bottomRegion.getWidth();

    filtersPanel.setFullSize (290, botFull);
    filtersPanel.setTopLeftPosition (0, 0);
    bandPanel.setFullSize (juce::jmax (420, W - 300), botFull);
    bandPanel.setTopLeftPosition (300, 0);

    // filtros de corte
    {
        auto fp = juce::Rectangle<int> (0, 0, 290, botFull).reduced (16, 14);
        fp.removeFromTop (26);
        const int half = fp.getWidth() / 2;

        auto layoutCut = [] (juce::Rectangle<int> c, juce::Label& l, CyberButton& on, juce::Slider& f, juce::ComboBox& sl)
        {
            auto head = c.removeFromTop (28);
            l.setBounds (head.removeFromLeft (40));
            on.setBounds (head.reduced (4, 0));
            sl.setBounds (c.removeFromBottom (28).reduced (4, 0));
            c.removeFromBottom (6);
            f.setBounds (c);
        };
        layoutCut (fp.removeFromLeft (half), lblHpf, hpfOn, hpfFreq, hpfSlope);
        layoutCut (fp, lblLpf, lpfOn, lpfFreq, lpfSlope);
    }

    // banda seleccionada
    {
        auto bp = juce::Rectangle<int> (0, 0, bandPanel.getWidth(), botFull).reduced (16, 14);
        bp.removeFromTop (26);

        auto left = bp.removeFromLeft (170);
        bType.setBounds  (left.removeFromTop (30));  left.removeFromTop (8);
        bandOn.setBounds (left.removeFromTop (30));  left.removeFromTop (8);
        dynBtn.setBounds (left.removeFromTop (30));  left.removeFromTop (8);
        addBtn.setBounds (left.removeFromTop (30));  left.removeFromTop (8);
        delBtn.setBounds (left.removeFromTop (30));
        bp.removeFromLeft (12);

        const int kw = bp.getWidth() / 7;
        auto caps = bp.removeFromTop (16);
        capFilter = caps.removeFromLeft (kw * 3);
        capDyn = caps;

        auto place = [&] (juce::Label& l, juce::Slider& s)
        {
            auto col = bp.removeFromLeft (kw);
            l.setBounds (col.removeFromTop (18));
            s.setBounds (col);
        };
        place (lblFreq, bFreq);  place (lblGain, bGain);  place (lblQ, bQ);
        place (lblRange, dRange); place (lblThr, dThr);   place (lblAtk, dAtk);  place (lblRel, dRel);
    }
}

void ParaEQEditor::resized()
{
    const int w = juce::jmax (1, getWidth()), h = juce::jmax (1, getHeight());
    bgCache = juce::Image (juce::Image::RGB, w, h, false);
    {
        juce::Graphics g (bgCache);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0e0919), 0.f, 0.f, juce::Colour (0xff04020a), 0.f, (float) h, false));
        g.fillAll();
        g.setColour (juce::Colours::black.withAlpha (0.20f));      // líneas de escaneo
        for (int y = 0; y < h; y += 3)
            g.drawHorizontalLine (y, 0.f, (float) w);
    }

    info.setBounds (getLocalBounds().reduced (60, 40));
    layoutMain();
}

void ParaEQEditor::paint (juce::Graphics& g)
{
    if (bgCache.isValid())
        g.drawImageAt (bgCache, 0, 0);
    else
        g.fillAll (Cyber::bg);

    g.setColour (juce::Colours::white.withAlpha (0.30f));
    g.setFont (Cyber::font (11.f, true));
    g.drawText ("SEMEPARAEQ", footer.withTrimmedRight (210), juce::Justification::centredLeft);
}

//==============================================================================
//  Teclado: ◀ ▶ cambian de banda, Cmd/Ctrl+Z deshace, Cmd/Ctrl+Shift+Z rehace
//==============================================================================
bool ParaEQEditor::keyPressed (const juce::KeyPress& k)
{
    using juce::KeyPress;
    using juce::ModifierKeys;

    if (k == KeyPress ('z', ModifierKeys (ModifierKeys::commandModifier | ModifierKeys::shiftModifier), 0)
        || k == KeyPress ('y', ModifierKeys (ModifierKeys::commandModifier), 0))
    {
        proc.redo();
        graph.refreshSelection();
        return true;
    }
    if (k == KeyPress ('z', ModifierKeys (ModifierKeys::commandModifier), 0))
    {
        proc.undo();
        graph.refreshSelection();
        return true;
    }
    if (k == KeyPress (KeyPress::leftKey))  { graph.selectNeighbor (-1); return true; }
    if (k == KeyPress (KeyPress::rightKey)) { graph.selectNeighbor (1);  return true; }
    if (k == KeyPress (KeyPress::deleteKey) || k == KeyPress (KeyPress::backspaceKey))
    {
        graph.deleteSelected();
        return true;
    }
    return false;
}

//==============================================================================
//  Presets y borrar todo
//==============================================================================
void ParaEQEditor::runProgrammatic (std::function<void()> change)
{
    proc.historyCheckpoint();
    change();
    proc.historyAfterProgrammatic();
    graph.refreshSelection();
}

void ParaEQEditor::setPresetName (const juce::String& n)
{
    currentPreset = n;
    presetBtn.setButtonText (n.isEmpty() ? juce::String ("PRESETS") : "PRESET: " + n);
}

void ParaEQEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& list = Presets::factory();

    juce::PopupMenu sub;
    juce::String cat;
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto c = Cyber::u8 (list[i].category);
        if (c != cat)
        {
            if (cat.isNotEmpty()) menu.addSubMenu (cat, sub);
            sub = juce::PopupMenu();
            cat = c;
        }
        sub.addItem (1000 + (int) i, Cyber::u8 (list[i].name));
    }
    if (cat.isNotEmpty()) menu.addSubMenu (cat, sub);

    menu.addSeparator();

    const auto files = Presets::userFiles();
    juce::PopupMenu mine;
    if (files.empty())
        mine.addItem (9, Cyber::u8 ("(vacío)"), false, false);
    for (size_t i = 0; i < files.size(); ++i)
        mine.addItem (2000 + (int) i, files[i].getFileNameWithoutExtension());
    menu.addSubMenu (Cyber::u8 ("Mis presets"), mine);

    menu.addSeparator();
    menu.addItem (1, "Guardar preset actual...");
    menu.addItem (2, "Cargar desde archivo...");
    menu.addItem (3, "Abrir carpeta de presets");

    juce::Component::SafePointer<ParaEQEditor> self (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [self, files] (int r)
    {
        if (self == nullptr || r == 0)
            return;

        if (r >= 2000)
        {
            self->loadPresetFile (files[(size_t) (r - 2000)]);
        }
        else if (r >= 1000)
        {
            const auto& p = Presets::factory()[(size_t) (r - 1000)];
            self->runProgrammatic ([self, &p] { Presets::apply (self->proc.apvts, p); });
            self->setPresetName (Cyber::u8 (p.name));
        }
        else if (r == 1)
        {
            self->askSavePreset();
        }
        else if (r == 2)
        {
            self->fileChooser = std::make_unique<juce::FileChooser> ("Elige un preset", Presets::userDir(), "*.paraeq");
            self->fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                            [self] (const juce::FileChooser& fc)
                                            {
                                                const auto f = fc.getResult();
                                                if (self != nullptr && f.existsAsFile())
                                                    self->loadPresetFile (f);
                                            });
        }
        else if (r == 3)
        {
            Presets::userDir().createDirectory();
            Presets::userDir().revealToUser();
        }
    });
}

void ParaEQEditor::loadPresetFile (const juce::File& f)
{
    if (auto xml = juce::parseXML (f))
    {
        runProgrammatic ([this, &xml] { Presets::applyXml (proc.apvts, *xml); });
        setPresetName (f.getFileNameWithoutExtension());
    }
}

void ParaEQEditor::askSavePreset()
{
    auto* w = new juce::AlertWindow (Cyber::u8 ("Guardar preset"), Cyber::u8 ("Escribe un nombre para tu preset:"),
                                     juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", currentPreset.isEmpty() ? juce::String ("Mi preset") : currentPreset, juce::String());
    w->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<ParaEQEditor> self (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([self, w] (int result)
    {
        if (result != 1 || self == nullptr)
            return;

        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isNotEmpty() && Presets::saveUser (self->proc.apvts, name))
            self->setPresetName (name);
    }), true);
}

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "EqEngine.h"

inline juce::String bandId (int band, const juce::String& name)
{
    return "b" + juce::String (band + 1) + "_" + name;
}

class ParaEQProcessor : public juce::AudioProcessor
{
public:
    ParaEQProcessor();
    ~ParaEQProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SemeParaEQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- Historial de deshacer / rehacer (solo desde el hilo de la interfaz)
    void historyTick (bool rawInteracting);    // llamar periódicamente desde el editor
    void historyTouch();                       // "el usuario está editando ahora"
    void historyCheckpoint();                  // antes de un cambio hecho por código (preset, borrar todo)
    void historyAfterProgrammatic();           // justo después de ese cambio
    bool canUndo() const { return dirty || ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }
    void undo();
    void redo();

    // ---- Acceso rápido a parámetros (la interfaz los lee sin buscar por nombre)
    struct BandPtrs  { std::atomic<float> *used, *on, *type, *freq, *gain, *q, *dyn, *dRange, *dThr, *dAtk, *dRel; };
    struct BandParam { juce::RangedAudioParameter *used, *on, *type, *freq, *gain, *q, *dyn, *dRange, *dThr, *dAtk, *dRel; };
    std::array<BandPtrs, EqMath::numBands> pBand {};
    std::array<BandParam, EqMath::numBands> bandParam {};
    std::atomic<float> *pIn = nullptr, *pOut = nullptr, *pBypass = nullptr, *pPolarity = nullptr,
                       *pHpfOn = nullptr, *pHpfFreq = nullptr, *pHpfSlope = nullptr,
                       *pLpfOn = nullptr, *pLpfFreq = nullptr, *pLpfSlope = nullptr;

    // ---- Datos para la interfaz
    std::atomic<bool> uiOpen { false };        // sin editor abierto no se calcula nada de esto
    std::array<std::atomic<float>, 2> inPk, inMs, outPk, outMs;
    std::array<std::atomic<float>, EqMath::numBands> dynShown;
    std::atomic<double> currentSr { 44100.0 };

    static constexpr int fftOrder  = 12;
    static constexpr int fftSize   = 1 << fftOrder;
    static constexpr int scopeSize = 1 << 13;
    std::array<std::atomic<float>, scopeSize> scope;
    std::atomic<int> scopePos { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    EqParams readParams() const;
    template <typename T> void process (juce::AudioBuffer<T>&);

    EqEngine engine;

    using Snapshot = std::vector<float>;
    Snapshot takeSnapshot() const;
    void restoreSnapshot (const Snapshot&);
    void flushPending();
    std::vector<Snapshot> undoStack, redoStack;
    Snapshot committed, lastSnap, preEdit;
    bool dirty = false;
    juce::uint32 lastChange = 0, touchMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParaEQProcessor)
};

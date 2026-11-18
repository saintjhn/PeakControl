#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace
{
    // Curva de clipping de una muestra
    inline float clipSample (float x, float ceiling, bool soft)
    {
        return soft ? ceiling * std::tanh (x / ceiling)
                    : juce::jlimit (-ceiling, ceiling, x);
    }

    // El ceiling se lee de una rampa con una muestra por cada muestra del bloque original.
    // "shift" convierte el indice sobremuestreado en indice original (0, 1, 2 o 3).
    void clipBlock (juce::dsp::AudioBlock<float>& block, const float* ceilRamp,
                    int rampSize, int shift, bool soft)
    {
        const int lastIdx = rampSize - 1;

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer (ch);

            for (size_t n = 0; n < block.getNumSamples(); ++n)
            {
                const int idx = juce::jmin ((int) (n >> shift), lastIdx);
                data[n] = clipSample (data[n], ceilRamp[idx], soft);
            }
        }
    }

    constexpr float maxLookaheadMs = 5.0f;
    constexpr int   dryBufSize     = 65536;   // anillo de la senal seca
    constexpr int   minRampSize    = 16384;   // tamano minimo de la rampa del ceiling
}

//==============================================================================
PeakControl_VST3AudioProcessor::PeakControl_VST3AudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

PeakControl_VST3AudioProcessor::~PeakControl_VST3AudioProcessor()
{
    cancelPendingUpdate();
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PeakControl_VST3AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 }, "Input Gain",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ceiling", 1 }, "Ceiling",
        juce::NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -0.3f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Clip Mode",
        juce::StringArray { "Soft (tanh)", "Hard" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oversampling", 1 }, "Oversampling",
        juce::StringArray { "Off", "2x", "4x", "8x" }, 1));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "limiterOn", 1 }, "Limiter", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lookahead", 1 }, "Lookahead (ms)",
        juce::NormalisableRange<float> (0.5f, maxLookaheadMs, 0.1f), 1.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release (ms)",
        juce::NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.4f), 80.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 }, "Output Gain (dB)",
        juce::NormalisableRange<float> (-24.0f, 0.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix (%)",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

//==============================================================================
const juce::String PeakControl_VST3AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PeakControl_VST3AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PeakControl_VST3AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PeakControl_VST3AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PeakControl_VST3AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PeakControl_VST3AudioProcessor::getNumPrograms()
{
    return 1;
}

int PeakControl_VST3AudioProcessor::getCurrentProgram()
{
    return 0;
}

void PeakControl_VST3AudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PeakControl_VST3AudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PeakControl_VST3AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PeakControl_VST3AudioProcessor::handleAsyncUpdate()
{
    // Se ejecuta en el hilo principal: aqui si es correcto avisar de la latencia
    setLatencySamples (pendingLatency.load());
}

//==============================================================================
void PeakControl_VST3AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = samplesPerBlock;
    const auto numChannels = (size_t) juce::jmax (1, getTotalNumInputChannels());

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            numChannels,
            i + 1,   // 1 etapa = 2x, 2 etapas = 4x, 3 etapas = 8x
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true,    // maxima calidad
            true);   // latencia en numero entero de muestras

        oversamplers[i]->initProcessing ((size_t) samplesPerBlock);
    }

    // Memoria del limiter para el lookahead maximo (5 ms)
    const int maxLook = (int) std::ceil (maxLookaheadMs * 0.001 * sampleRate) + 2;
    delayBuffer.setSize (2, maxLook + 1);
    targetBuf.assign ((size_t) maxLook + 1, 1.0f);
    smoothBuf.assign ((size_t) maxLook + 1, 1.0f);

    // Senal seca para Dry/Wet y bypass
    dryBuffer.setSize (2, dryBufSize);
    dryBuffer.clear();
    dryWritePos = 0;

    // Suavizado de Input Gain, Ceiling, Mix y Output Gain (20 ms)
    const float ceilingNow = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("ceiling")->load());
    ceilingRamp.assign ((size_t) juce::jmax (samplesPerBlock, minRampSize), ceilingNow);

    inGainSmoothed.reset (sampleRate, 0.02);
    inGainSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("inputGain")->load()));

    ceilingSmoothed.reset (sampleRate, 0.02);
    ceilingSmoothed.setCurrentAndTargetValue (ceilingNow);

    mixSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load() * 0.01f);

    outGainSmoothed.reset (sampleRate, 0.02);
    outGainSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outputGain")->load()));

    // Fuerza que se recalcule todo (latencia, limiter) en el siguiente bloque
    lastOsChoice = -1;
    lastLookaheadSamples = -1;
    lastLimiterOn = false;
    lastLatency = -1;
    wasPlaying = false;
    resetLimiter (0);
}

void PeakControl_VST3AudioProcessor::releaseResources()
{
}

bool PeakControl_VST3AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
void PeakControl_VST3AudioProcessor::resetLimiter (int newLookaheadSamples)
{
    lookaheadSamples = newLookaheadSamples;
    const int len = lookaheadSamples + 1;

    delayBuffer.clear();
    std::fill (targetBuf.begin(), targetBuf.begin() + len, 1.0f);
    std::fill (smoothBuf.begin(), smoothBuf.begin() + len, 1.0f);
    smoothSum = (double) len;
    smoothPrev = 1.0f;
    ringPos = 0;
}

// Se llama al arrancar la reproduccion: limpia limiter, senal seca y oversampling
void PeakControl_VST3AudioProcessor::resetProcessingState()
{
    resetLimiter (lookaheadSamples);
    dryBuffer.clear();

    if (lastOsChoice > 0 && oversamplers[(size_t) lastOsChoice - 1] != nullptr)
        oversamplers[(size_t) lastOsChoice - 1]->reset();
}

// Limiter: retardo + ganancia objetivo + minimo en ventana + release + media movil.
// La media movil de longitud igual al retardo garantiza que la ganancia ya
// ha bajado del todo cuando el pico llega a la salida.
void PeakControl_VST3AudioProcessor::applyLimiter (juce::AudioBuffer<float>& buffer, int numChannels,
                                                   const float* ceilRamp, int rampSize, float releaseCoef)
{
    const int len = lookaheadSamples + 1;
    const int numSamples = buffer.getNumSamples();
    const int chans = juce::jmin (numChannels, 2);

    for (int n = 0; n < numSamples; ++n)
    {
        const float ceiling = ceilRamp[juce::jmin (n, rampSize - 1)];

        // 1) Pico entre canales (enlazado) y ganancia objetivo
        float peak = 0.0f;
        for (int ch = 0; ch < chans; ++ch)
            peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[n]));

        const float target = peak > ceiling ? ceiling / peak : 1.0f;

        // 2) Guarda objetivo y audio en el anillo
        const int p = ringPos;
        targetBuf[(size_t) p] = target;

        for (int ch = 0; ch < chans; ++ch)
            delayBuffer.getWritePointer (ch)[p] = buffer.getReadPointer (ch)[n];

        // 3) Minimo de la ventana (ultimas L+1 muestras)
        float minHeld = 1.0f;
        for (int i = 0; i < len; ++i)
            minHeld = juce::jmin (minHeld, targetBuf[(size_t) i]);

        // 4) Release: baja al instante, sube poco a poco
        const float r = (minHeld < smoothPrev) ? minHeld
                                               : minHeld + releaseCoef * (smoothPrev - minHeld);
        smoothPrev = r;

        // 5) Media movil de la envolvente (suaviza el ataque)
        smoothSum += (double) r - (double) smoothBuf[(size_t) p];
        smoothBuf[(size_t) p] = r;

        const float gain = juce::jmin (1.0f, (float) (smoothSum / (double) len));

        // 6) Audio retrasado L muestras, con la ganancia aplicada
        const int readPos = (p + 1) % len;

        for (int ch = 0; ch < chans; ++ch)
        {
            const float out = delayBuffer.getReadPointer (ch)[readPos] * gain;
            buffer.getWritePointer (ch)[n] = juce::jlimit (-ceiling, ceiling, out);   // seguridad por redondeos
        }

        ringPos = (p + 1) % len;
    }
}

//==============================================================================
void PeakControl_VST3AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    // --- Lectura de parametros ---
    const float inGainTarget  = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("inputGain")->load());
    const float ceilingTarget = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("ceiling")->load());
    const bool  soft          = apvts.getRawParameterValue ("mode")->load() < 0.5f;
    int osChoice              = (int) apvts.getRawParameterValue ("oversampling")->load();   // 0 = Off, 1 = 2x, 2 = 4x, 3 = 8x

    const bool  limiterOn  = apvts.getRawParameterValue ("limiterOn")->load() > 0.5f;
    const float lookMs     = apvts.getRawParameterValue ("lookahead")->load();
    const float releaseMs  = apvts.getRawParameterValue ("release")->load();
    const bool  bypassed   = apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    // Bypass = fundido a la senal original (seca, con la misma latencia)
    mixSmoothed.setTargetValue (bypassed ? 0.0f : apvts.getRawParameterValue ("mix")->load() * 0.01f);
    outGainSmoothed.setTargetValue (bypassed ? 1.0f
        : juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outputGain")->load()));
    inGainSmoothed.setTargetValue (inGainTarget);
    ceilingSmoothed.setTargetValue (ceilingTarget);

    // Si el bloque es mayor de lo previsto, procesamos sin oversampling por seguridad
    if (numSamples > maxBlockSize || oversamplers[0] == nullptr)
        osChoice = 0;

    // --- Proteccion: elimina NaN e infinitos de la entrada ---
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        float* d = buffer.getWritePointer (ch);

        for (int n = 0; n < numSamples; ++n)
            if (! std::isfinite (d[n]))
                d[n] = 0.0f;
    }

    // --- Cambio de oversampling ---
    if (osChoice != lastOsChoice)
    {
        lastOsChoice = osChoice;

        if (osChoice > 0)
        {
            oversamplers[(size_t) osChoice - 1]->reset();
            osLatency = juce::roundToInt (oversamplers[(size_t) osChoice - 1]->getLatencyInSamples());
        }
        else
        {
            osLatency = 0;
        }
    }

    // --- Cambio de lookahead / limiter on-off ---
    const int newLook = juce::jlimit (1, delayBuffer.getNumSamples() - 2,
                                      juce::roundToInt (lookMs * 0.001f * (float) currentSampleRate));

    if (limiterOn != lastLimiterOn || (limiterOn && newLook != lastLookaheadSamples))
    {
        lastLimiterOn = limiterOn;
        lastLookaheadSamples = newLook;
        resetLimiter (newLook);
    }

    // --- Al arrancar la reproduccion, limpia el estado interno ---
    bool playing = true;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            playing = pos->getIsPlaying();

    if (playing && ! wasPlaying)
        resetProcessingState();

    wasPlaying = playing;

    // --- Latencia total: se avisa al host desde el hilo principal ---
    const int totalLatency = osLatency + (limiterOn ? lookaheadSamples : 0);

    if (totalLatency != lastLatency)
    {
        lastLatency = totalLatency;
        pendingLatency.store (totalLatency);
        triggerAsyncUpdate();
    }

    // --- Rampa del ceiling suavizado (una muestra por cada muestra del bloque) ---
    const int rampSize = (int) ceilingRamp.size();
    const int rampFill = juce::jmin (numSamples, rampSize);

    for (int n = 0; n < rampFill; ++n)
        ceilingRamp[(size_t) n] = ceilingSmoothed.getNextValue();

    if (numSamples > rampFill)
        ceilingSmoothed.skip (numSamples - rampFill);

    // --- Guarda la senal seca (entrada original) en el anillo ---
    const int chans = juce::jmin (totalNumInputChannels, 2);

    for (int ch = 0; ch < chans; ++ch)
    {
        const float* in = buffer.getReadPointer (ch);
        float* ring = dryBuffer.getWritePointer (ch);

        for (int n = 0; n < numSamples; ++n)
            ring[(dryWritePos + n) % dryBufSize] = in[n];
    }

    // --- Ganancia de entrada (suavizada) ---
    for (int n = 0; n < numSamples; ++n)
    {
        const float g = inGainSmoothed.getNextValue();

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
            buffer.getWritePointer (ch)[n] *= g;
    }

    // --- Clipper (con o sin oversampling) ---
    juce::dsp::AudioBlock<float> block (buffer);
    auto activeBlock = block.getSubsetChannelBlock (0, (size_t) totalNumInputChannels);

    if (osChoice == 0)
    {
        clipBlock (activeBlock, ceilingRamp.data(), rampSize, 0, soft);
    }
    else
    {
        auto& os = *oversamplers[(size_t) osChoice - 1];

        auto upBlock = os.processSamplesUp (activeBlock);
        clipBlock (upBlock, ceilingRamp.data(), rampSize, osChoice, soft);   // 2x -> shift 1, 4x -> 2, 8x -> 3
        os.processSamplesDown (activeBlock);
    }

    // --- Limiter ---
    if (limiterOn)
    {
        const float releaseCoef = std::exp (-1.0f / (0.001f * releaseMs * (float) currentSampleRate));
        applyLimiter (buffer, totalNumInputChannels, ceilingRamp.data(), rampSize, releaseCoef);
    }

    // --- Dry/Wet (la seca se retrasa la misma latencia que la procesada) + Output Gain ---
    const bool dryAvailable = (numSamples + totalLatency) < dryBufSize;

    for (int n = 0; n < numSamples; ++n)
    {
        const float mix = dryAvailable ? mixSmoothed.getNextValue() : 1.0f;
        const float outG = outGainSmoothed.getNextValue();

        const int readIdx = ((dryWritePos + n - totalLatency) % dryBufSize + dryBufSize) % dryBufSize;

        for (int ch = 0; ch < chans; ++ch)
        {
            const float wet = buffer.getReadPointer (ch)[n];
            const float dry = dryBuffer.getReadPointer (ch)[readIdx];

            buffer.getWritePointer (ch)[n] = (dry * (1.0f - mix) + wet * mix) * outG;
        }
    }

    dryWritePos = (dryWritePos + numSamples) % dryBufSize;
}

//==============================================================================
bool PeakControl_VST3AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PeakControl_VST3AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void PeakControl_VST3AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PeakControl_VST3AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PeakControl_VST3AudioProcessor();
}

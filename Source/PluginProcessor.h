#pragma once

#include <JuceHeader.h>

//==============================================================================
class PeakControl_VST3AudioProcessor : public juce::AudioProcessor,
                                       private juce::AsyncUpdater
{
public:
    //==============================================================================
    PeakControl_VST3AudioProcessor();
    ~PeakControl_VST3AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==============================================================================
    // La latencia se comunica al host desde el hilo principal
    void handleAsyncUpdate() override;
    std::atomic<int> pendingLatency { 0 };

    // --- Oversampling: [0] = 2x, [1] = 4x, [2] = 8x ---
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    int lastOsChoice = -1;
    int maxBlockSize = 0;
    int osLatency = 0;

    // --- Limiter con lookahead ---
    void resetLimiter (int newLookaheadSamples);
    void applyLimiter (juce::AudioBuffer<float>& buffer, int numChannels,
                       const float* ceilRamp, int rampSize, float releaseCoef);
    void resetProcessingState();

    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> delayBuffer;   // retardo del audio (2 canales)
    std::vector<float> targetBuf;           // ganancia objetivo de las ultimas L+1 muestras
    std::vector<float> smoothBuf;           // envolvente con release de las ultimas L+1 muestras
    double smoothSum = 0.0;
    float  smoothPrev = 1.0f;
    int    ringPos = 0;
    int    lookaheadSamples = 0;
    int    lastLookaheadSamples = -1;
    bool   lastLimiterOn = false;
    int    lastLatency = -1;
    bool   wasPlaying = false;

    // --- Dry/Wet, Output Gain, Input Gain y Ceiling ---
    juce::AudioBuffer<float> dryBuffer;     // anillo con la senal seca (2 canales)
    int dryWritePos = 0;
    std::vector<float> ceilingRamp;         // ceiling suavizado, una muestra por cada muestra del bloque
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ceilingSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakControl_VST3AudioProcessor)
};

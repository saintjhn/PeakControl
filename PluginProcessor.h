#pragma once

#include <JuceHeader.h>

//==============================================================================
class PeakControl_VST3AudioProcessor : public juce::AudioProcessor
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
    // --- Oversampling: [0] = 2x, [1] = 4x, [2] = 8x ---
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    int lastOsChoice = -1;
    int maxBlockSize = 0;
    int osLatency = 0;

    // --- Limiter con lookahead ---
    void resetLimiter(int newLookaheadSamples);
    void applyLimiter(juce::AudioBuffer<float>& buffer, int numChannels, float ceiling, float releaseCoef);

    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> delayBuffer;   // retardo del audio (2 canales)
    std::vector<float> targetBuf;           // ganancia objetivo de las últimas L+1 muestras
    std::vector<float> smoothBuf;           // envolvente con release de las últimas L+1 muestras
    double smoothSum = 0.0;
    float  smoothPrev = 1.0f;
    int    ringPos = 0;
    int    lookaheadSamples = 0;
    int    lastLookaheadSamples = -1;
    bool   lastLimiterOn = false;
    int    lastLatency = -1;

    // --- Dry/Wet y Output Gain ---
    juce::AudioBuffer<float> dryBuffer;     // anillo con la señal seca (2 canales)
    int dryWritePos = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outGainSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakControl_VST3AudioProcessor)
};
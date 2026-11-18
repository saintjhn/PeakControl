#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class PeakControl_VST3AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    PeakControl_VST3AudioProcessorEditor(PeakControl_VST3AudioProcessor&);
    ~PeakControl_VST3AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PeakControl_VST3AudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakControl_VST3AudioProcessorEditor)
};
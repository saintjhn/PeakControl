#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PeakControl_VST3AudioProcessorEditor::PeakControl_VST3AudioProcessorEditor(PeakControl_VST3AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(400, 300);
}

PeakControl_VST3AudioProcessorEditor::~PeakControl_VST3AudioProcessorEditor()
{
}

//==============================================================================
void PeakControl_VST3AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PeakControl_VST3AudioProcessorEditor::resized()
{
}
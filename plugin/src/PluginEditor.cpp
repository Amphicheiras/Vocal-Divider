#include "VocalDivider/PluginEditor.h"
#include "VocalDivider/PluginProcessor.h"

PluginEditor::PluginEditor(PluginProcessor &p) : AudioProcessorEditor(&p), audioProcessor(p)
{
    juce::ignoreUnused(audioProcessor);

    setSize(400, 300);

    addAndMakeVisible(fundamentalFrequencyLabel);
    fundamentalFrequencyLabel.setJustificationType(juce::Justification::centred);
    fundamentalFrequencyLabel.setText("Fundamental Frequency: 0 Hz", juce::dontSendNotification);

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Vocal Divider says hi!", getLocalBounds(), juce::Justification::centred, 1);
}

void PluginEditor::resized()
{
    fundamentalFrequencyLabel.setBounds(10, 40, getWidth() - 20, 40);
}

void PluginEditor::timerCallback()
{
    fundamentalFrequencyLabel.setText("Fundamental Frequency: " + juce::String(audioProcessor.getFundamentalFrequency()) + " Hz", juce::dontSendNotification);
}
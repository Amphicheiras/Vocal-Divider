#include "VocalDivider/PluginEditor.h"
#include "VocalDivider/PluginProcessor.h"

PluginEditor::PluginEditor(PluginProcessor &p) : AudioProcessorEditor(&p), audioProcessor(p)
{
    juce::ignoreUnused(audioProcessor);

    setSize(400, 300);

    addAndMakeVisible(leftFundamentalFrequencyLabel);
    leftFundamentalFrequencyLabel.setJustificationType(juce::Justification::centred);
    leftFundamentalFrequencyLabel.setText("Fundamental Frequency: 0 Hz", juce::dontSendNotification);

    addAndMakeVisible(rightFundamentalFrequencyLabel);
    rightFundamentalFrequencyLabel.setJustificationType(juce::Justification::centred);
    rightFundamentalFrequencyLabel.setText("Fundamental Frequency: 0 Hz", juce::dontSendNotification);

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
    leftFundamentalFrequencyLabel.setBounds(10, 40, getWidth() - 20, 40);
    rightFundamentalFrequencyLabel.setBounds(10, 50, getWidth() - 20, 50);
}

void PluginEditor::timerCallback()
{
    leftFundamentalFrequencyLabel.setText("Fundamental Frequency #1: " + juce::String(audioProcessor.getFundamentalFrequency(0)) + " Hz", juce::dontSendNotification);
    rightFundamentalFrequencyLabel.setText("Fundamental Frequency #2: " + juce::String(audioProcessor.getFundamentalFrequency(1)) + " Hz", juce::dontSendNotification);
}
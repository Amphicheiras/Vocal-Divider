#include "VocalDivider/PluginEditor.h"
#include "VocalDivider/PluginProcessor.h"

PluginEditor::PluginEditor(PluginProcessor &p) : AudioProcessorEditor(&p), audioProcessor(p)
{
    juce::ignoreUnused(audioProcessor);

    setSize(400, 300);

    addAndMakeVisible(leftFundamentalFrequencyLabel);
    leftFundamentalFrequencyLabel.setJustificationType(juce::Justification::centred);
    leftFundamentalFrequencyLabel.setText("Fundamental Frequency: 0 Hz", juce::dontSendNotification);
    // leftFundamentalFrequencyLabel.setColour(juce::Colours::black);

    // Set font size and color for the left label
    // leftFundamentalFrequencyLabel.setFont(juce::Font(16.0f, juce::Font::bold));               // Change size to 16 and make it bold
    leftFundamentalFrequencyLabel.setColour(juce::Label::textColourId, juce::Colours::white); // Set text color to black

    addAndMakeVisible(rightFundamentalFrequencyLabel);
    rightFundamentalFrequencyLabel.setJustificationType(juce::Justification::centred);
    rightFundamentalFrequencyLabel.setText("Fundamental Frequency: 0 Hz", juce::dontSendNotification);

    // Set font size and color for the left label
    // rightFundamentalFrequencyLabel.setFont(juce::Font(16.0f, juce::Font::bold));               // Change size to 16 and make it bold
    rightFundamentalFrequencyLabel.setColour(juce::Label::textColourId, juce::Colours::wheat); // Set text color to black

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::blueviolet);
    g.setColour(juce::Colours::black);
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
#pragma once

#include "VocalDivider/PluginProcessor.h"

class PluginEditor final : public juce::AudioProcessorEditor,
                           public juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor &);
    ~PluginEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void timerCallback() override;

private:
    PluginProcessor &audioProcessor;

    juce::Label leftFundamentalFrequencyLabel;
    juce::Label rightFundamentalFrequencyLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
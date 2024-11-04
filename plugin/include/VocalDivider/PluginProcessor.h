#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "VocalDivider/MultiBandFilter.h"

class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &midiMessages);
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    float getFundamentalFrequency(int channel) const
    {
        return fundamentalFrequency[channel];
    }

private:
    juce::dsp::FFT fft;
    juce::AudioBuffer<float> fftBuffer;
    static constexpr int fftOrder = 10; // 2^10 = 1024-point FFT
    static constexpr int fftSize = 1 << fftOrder;
    std::vector<float> fundamentalFrequency;

    MultiBandFilter multiBandFilter;

    double PI = 3.1415926;
    double lastSampleRate;
    juce::dsp::IIR::Filter<float> leftBandpassFilter;
    juce::dsp::IIR::Filter<float> rightBandpassFilter;
    void updateFilter(float freq, float res, bool channel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
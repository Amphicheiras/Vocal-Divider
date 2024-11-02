#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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

    int getFundamentalFrequency(int channel) const
    {
        return fundamentalFrequency[channel];
    }

private:
    juce::dsp::FFT fft;
    juce::AudioBuffer<float> fftBuffer;
    static constexpr int fftOrder = 9; // 2^10 = 1024-point FFT
    static constexpr int fftSize = 1 << fftOrder;
    std::vector<int> fundamentalFrequency;
    void applyBandpassFilter(juce::AudioBuffer<float> &buffer, int channel, int centerFrequency, float bandwidth);

    std::vector<double> x1;
    std::vector<double> x2;
    std::vector<double> y1;
    std::vector<double> y2;
    double PI = 3.1415926;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
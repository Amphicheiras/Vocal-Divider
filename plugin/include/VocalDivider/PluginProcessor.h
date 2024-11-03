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
    static constexpr int fftOrder = 10; // 2^10 = 1024-point FFT
    static constexpr int fftSize = 1 << fftOrder;
    std::vector<int> fundamentalFrequency;

    double PI = 3.1415926;
    double lastSampleRate;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bandpassFilter;
    void updateFilter();
    float centerFrequency = 1000.0f; // Default center frequency in Hz
    float Q = 0.707f;                // Default Q factor
    void setCenterFrequency(float newFrequency);
    void setQFactor(float newQ);

    // void applyBandpassFilter(juce::AudioBuffer<float> &buffer, int centerFrequency, int bandwidth);
    // void applyMultiBandPassFilter(juce::AudioBuffer<float> &buffer, int channel, int f0);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

class MultiBandFilter
{
public:
    MultiBandFilter();
    void prepare(const juce::dsp::ProcessSpec &spec);
    void addBand(double sampleRate, double centerFrequency, double qualityFactor, int order);
    void processSingleChannel(juce::AudioBuffer<float> &buffer, int channel);

private:
    class BandpassFilter
    {
    public:
        BandpassFilter() = default;
        void prepare(const juce::dsp::ProcessSpec &spec);
        void setBandpassParameters(double sampleRate, double frequency, double qualityFactor, int order);
        void processSingleChannel(juce::AudioBuffer<float> &buffer, int channel);

    private:
        std::vector<juce::dsp::IIR::Filter<float>> filters;
    };

    std::vector<BandpassFilter> bands;
};

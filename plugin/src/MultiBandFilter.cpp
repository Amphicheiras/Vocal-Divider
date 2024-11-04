#include "VocalDivider/MultiBandFilter.h"

MultiBandFilter::MultiBandFilter() = default;

void MultiBandFilter::prepare(const juce::dsp::ProcessSpec &spec)
{
    for (auto &band : bands)
        band.prepare(spec);
}

void MultiBandFilter::addBand(double sampleRate, double centerFrequency, double qualityFactor, int order)
{
    BandpassFilter newBand;
    newBand.setBandpassParameters(sampleRate, centerFrequency, qualityFactor, order);
    bands.push_back(std::move(newBand));
}

void MultiBandFilter::processSingleChannel(juce::AudioBuffer<float> &buffer, int channel)
{
    juce::AudioBuffer<float> tempBuffer;
    tempBuffer.setSize(1, buffer.getNumSamples());
    tempBuffer.clear();

    for (auto &band : bands)
    {
        tempBuffer.copyFrom(0, 0, buffer, channel, 0, buffer.getNumSamples());
        band.processSingleChannel(tempBuffer, 0);
        buffer.addFrom(channel, 0, tempBuffer, 0, 0, buffer.getNumSamples());
    }
}

// --------------------- BandpassFilter Subclass Implementation --------------------- //

void MultiBandFilter::BandpassFilter::prepare(const juce::dsp::ProcessSpec &spec)
{
    for (auto &filter : filters)
        filter.prepare(spec);
}

void MultiBandFilter::BandpassFilter::setBandpassParameters(double sampleRate, double frequency, double qualityFactor, int order)
{
    filters.resize(order / 2);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, frequency, qualityFactor);
    for (auto &filter : filters)
        filter.coefficients = coeffs;
}

void MultiBandFilter::BandpassFilter::processSingleChannel(juce::AudioBuffer<float> &buffer, int channel)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block.getSingleChannelBlock(channel));

    for (auto &filter : filters)
        filter.process(context);
}

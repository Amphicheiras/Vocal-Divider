#include "VocalDivider/PluginProcessor.h"
#include "VocalDivider/PluginEditor.h"

PluginProcessor::PluginProcessor() : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                                     fft(fftOrder),
                                     fftBuffer(2, fftSize),
                                     fundamentalFrequency(2, 0)
{
}

PluginProcessor::~PluginProcessor()
{
}

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0
              // programs, so this should be at least 1, even if you're not
              // really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index,
                                        const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;
    samplesPerBlock;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    leftBandpassFilter.prepare(spec);
    rightBandpassFilter.prepare(spec);
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    // **************** D O N T  T O U C H  T H A N K S! ****************
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);
    // ****************************************************

    fftBuffer.clear();
    // assume mono input (mixed stereo)
    fftBuffer.copyFrom(0, 0, buffer.getWritePointer(0), fftSize);
    // FFT
    fft.performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(0));
    // obtain f0
    std::vector<float> magnitudes(fftSize / 2);
    for (int i = 1; i < fftSize / 2; ++i)
    {
        magnitudes[i] = fftBuffer.getSample(0, i);
    }
    // find local maxima
    std::vector<int> localMaximaIndices;
    for (int i = 1; i < magnitudes.size() - 1; ++i)
    {
        // magnitude threshold
        if (magnitudes[i] > 5)
        {
            if (magnitudes[i] > magnitudes[i - 1] && magnitudes[i] > magnitudes[i + 1])
            {
                localMaximaIndices.push_back(i);
            }
        }
    }
    // identify fundamental frequency (f0)
    if (!localMaximaIndices.empty())
    {
        // first local maximum is f0_L
        fundamentalFrequency[0] = static_cast<float>(localMaximaIndices[0] * (getSampleRate() / fftSize));

        // second local maximum is f0_R
        if (localMaximaIndices.size() > 1)
        {
            fundamentalFrequency[1] = static_cast<float>(localMaximaIndices[1] * (getSampleRate() / fftSize));
        }
    }
    // apply bandpasses
    juce::dsp::AudioBlock<float> block(buffer);
    if (fundamentalFrequency[0] > 0)
    {
        updateFilter(fundamentalFrequency[0], 7.f, true);
        juce::dsp::AudioBlock<float> leftChannelBlock(block.getSingleChannelBlock(0));
        leftBandpassFilter.process(juce::dsp::ProcessContextReplacing<float>(leftChannelBlock));
    }
    if (fundamentalFrequency[1] > 0)
    {
        updateFilter(fundamentalFrequency[1], 7.f, false);
        juce::dsp::AudioBlock<float> rightChannelBlock(block.getSingleChannelBlock(1));
        rightBandpassFilter.process(juce::dsp::ProcessContextReplacing<float>(rightChannelBlock));
    }
}

bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void PluginProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory
    // block, whose contents will have been created by the getStateInformation()
    // call.
    juce::ignoreUnused(data, sizeInBytes);
}

// void PluginProcessor::applyBandpassFilter(juce::AudioBuffer<float> &buffer, float cutoffFrequency)
// {
//     std::vector<double> x1;
//     std::vector<double> x2;
//     std::vector<double> y1;
//     std::vector<double> y2;

//     x1.resize(buffer.getNumChannels(), 0.f);
//     x2.resize(buffer.getNumChannels(), 0.f);
//     y1.resize(buffer.getNumChannels(), 0.f);
//     y2.resize(buffer.getNumChannels(), 0.f);

//     // actual processing; each channel separately
//     for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
//     {
//         auto channelSamples = buffer.getWritePointer(channel);

//         const double tan = std::tan(PI * cutoffFrequency / getSampleRate());
//         const double c = (tan - 1) / (tan + 1);
//         const double d = -std::cos(2.f * PI * cutoffFrequency / getSampleRate());

//         std::vector<double> b = {-c, d * (1.f - c), 1.f};
//         std::vector<double> a = {1.f, d * (1.f - c), -c}; // resize the allpass buffers to the number of channels

//         // for each sample in the channel
//         for (auto i = 0; i < buffer.getNumSamples(); ++i)
//         {
//             double x = channelSamples[i];
//             double y = b[0] * x + b[1] * x1[channel] + b[2] * x2[channel] - a[1] * y1[channel] - a[2] * y2[channel];

//             y2[channel] = y1[channel];
//             y1[channel] = y;
//             x2[channel] = x1[channel];
//             x1[channel] = x;

//             // scale by 0.5 to stay in the [-1, 1] range
//             channelSamples[i] = 0.5f * static_cast<float>(x - y);
//         }
//     }
// }

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

void PluginProcessor::updateFilter(float freq, float res, bool channel)
{
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(44100, freq, res);
    if (channel)
        leftBandpassFilter.coefficients = coefficients;
    else
        rightBandpassFilter.coefficients = coefficients;
}
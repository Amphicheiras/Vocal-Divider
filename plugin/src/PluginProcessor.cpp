#include "VocalDivider/PluginProcessor.h"
#include "VocalDivider/PluginEditor.h"

PluginProcessor::PluginProcessor() : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                                     fft(fftOrder),
                                     fftBuffer(2, fftSize),
                                     fundamentalFrequency(2, 0)
{
}

PluginProcessor::~PluginProcessor() {}

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
    juce::ignoreUnused(sampleRate, samplesPerBlock);
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
    //  assume mono input (mixed stereo)
    fftBuffer.copyFrom(0, 0, buffer.getWritePointer(0), fftSize);
    // FFT
    fft.performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(0));

    // obtain f0
    // Find the magnitudes in the FFT
    std::vector<float> magnitudes(fftSize / 2);
    for (int i = 1; i < fftSize / 2; ++i)
    {
        magnitudes[i] = fftBuffer.getSample(0, i);
    }
    // Finding local maxima
    std::vector<int> localMaximaIndices;
    for (int i = 1; i < magnitudes.size() - 1; ++i)
    {
        // Check for local maxima
        if (magnitudes[i] > 5)
        {
            if (magnitudes[i] > magnitudes[i - 1] && magnitudes[i] > magnitudes[i + 1])
            {
                localMaximaIndices.push_back(i);
            }
        }
    }

    // If we found local maxima, identify the fundamental frequency (f0)
    if (!localMaximaIndices.empty())
    {
        DBG(magnitudes[localMaximaIndices[0]]);
        // Assuming the first local maximum is the fundamental frequency
        fundamentalFrequency[0] = static_cast<int>(localMaximaIndices[0] * (getSampleRate() / fftSize));

        // Assuming the second local maximum is the first harmonic
        if (localMaximaIndices.size() > 1)
        {
            DBG(magnitudes[localMaximaIndices[1]]);
            fundamentalFrequency[1] = static_cast<int>(localMaximaIndices[1] * (getSampleRate() / fftSize));
        }
    }

    // Debug output to see detected frequencies
    DBG("Fundamental Frequencies: F0: " + juce::String(fundamentalFrequency[0]) + " F1: " + juce::String(fundamentalFrequency[1]));
    // -------------------

    if (fundamentalFrequency[0] > 0)
        applyBandpassFilter(buffer, 0, fundamentalFrequency[0], 20.0f);
    if (fundamentalFrequency[1] > 0)
        applyBandpassFilter(buffer, 1, fundamentalFrequency[1], 20.0f);
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

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

void PluginProcessor::applyBandpassFilter(juce::AudioBuffer<float> &buffer, int channel, int centerFrequency, float bandwidth)
{
    // resize the allpass buffers to the number of channels and
    // zero the new ones
    x1.resize(1, 0.0);
    x2.resize(1, 0.0);
    y1.resize(1, 0.0);
    y2.resize(1, 0.0);

    const double normalizedCenterFreq = centerFrequency / getSampleRate();
    const double tanHalfBand = std::tan(PI * bandwidth / getSampleRate());
    const double c = (tanHalfBand - 1.0) / (tanHalfBand + 1.0);
    const double d = -std::cos(2.0 * PI * normalizedCenterFreq);

    std::vector<double> b = {-c, d * (1.f - c), 1.f};
    std::vector<double> a = {1.f, d * (1.f - c), -c};

    // const double Q = 5.0;
    // double BW = centerFrequency / Q;

    auto channelSamples = buffer.getWritePointer(channel);

    // for each sample in the channel
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        double x = static_cast<double>(channelSamples[i]);
        double y = b[0] * x + b[1] * x1[0] + b[2] * x2[0] - a[1] * y1[0] - a[2] * y2[0];

        y2[0] = y1[0];
        y1[0] = y;
        x2[0] = x1[0];
        x1[0] = x;

        // we scale by 0.5 to stay in the [-1, 1] range
        channelSamples[i] = static_cast<float>(0.5 * (x - y));
    }
}
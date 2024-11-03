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
        fundamentalFrequency[0] = static_cast<int>(localMaximaIndices[0] * (getSampleRate() / fftSize));

        // second local maximum is f0_R
        if (localMaximaIndices.size() > 1)
        {
            fundamentalFrequency[1] = static_cast<int>(localMaximaIndices[1] * (getSampleRate() / fftSize));
        }
    }
    // apply bandpass
    if (fundamentalFrequency[0] > 0)
        applyMultiBandPassFilter(buffer, 0, fundamentalFrequency[0]);
    if (fundamentalFrequency[1] > 0)
        applyMultiBandPassFilter(buffer, 1, fundamentalFrequency[1]);
    // apply bandpass
    // if (fundamentalFrequency[1] > 0)
    //     applyBandpassFilter(buffer, 1, 1500, 100);
    // if (fundamentalFrequency[0] > 0)
    //     applyBandpassFilter(buffer, 0, 1500, 100);
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

void PluginProcessor::applyBandpassFilter(juce::AudioBuffer<float> &buffer, int centerFrequency, int bandwidth)
{
    // State variables for the filter
    static double x1 = 0.0;
    static double x2 = 0.0;
    static double y1 = 0.0;
    static double y2 = 0.0;

    // Normalization
    const double sampleRate = getSampleRate();
    const double normalizedCenterFreq = centerFrequency / sampleRate;
    const double normalizedBandwidth = bandwidth / sampleRate;

    // Calculate Quality Factor (Q)
    const double Q = normalizedCenterFreq / normalizedBandwidth;

    // Calculate coefficients
    const double omega = 2.0 * PI * normalizedCenterFreq;
    const double alpha = std::sin(omega) / (2.0 * Q); // Use Q to calculate alpha

    const double a0 = 1.0 + alpha; // Normalization factor
    const double a1 = -2.0 * std::cos(omega);
    const double a2 = 1.0 - alpha;

    const double b0 = alpha;
    const double b1 = 0.0;
    const double b2 = -alpha;

    auto channelSamples = buffer.getWritePointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        double x = static_cast<double>(channelSamples[i]);

        // Apply the filter equation
        double y = (b0 / a0) * x + (b1 / a0) * x1 + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2;

        // Update state variables
        y2 = y1;
        y1 = y;
        x2 = x1;
        x1 = x;

        // Output sample
        channelSamples[i] = static_cast<float>(y); // Write the output directly
    }
}

void PluginProcessor::applyMultiBandPassFilter(juce::AudioBuffer<float> &buffer, int channel, int f0)
{
    const int numBands = 3;

    // Create temporary buffer for the output
    juce::AudioBuffer<float> tempBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    tempBuffer.clear();
    const std::vector<int> centerFrequencies = {f0 * 1, f0 * 2, f0 * 3}; // Hz
    const std::vector<int> bandwidths = {100, 100, 100};                 // Hz
    // Apply bandpass filters for each band
    for (int band = 0; band < numBands; ++band)
    {
        // Call the existing bandpass filter function for each frequency band
        applyBandpassFilter(tempBuffer, centerFrequencies[band], bandwidths[band]);
    }

    // Combine outputs
    auto channelSamples = buffer.getWritePointer(channel);
    auto tempSamples = tempBuffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Here you can sum the outputs from each band, or use a different combining method
        channelSamples[i] += tempSamples[i]; // Simple summation, adjust based on needs
    }
}

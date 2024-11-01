#include "VocalDivider/PluginProcessor.h"
#include "VocalDivider/PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      fft(fftOrder),
      fftBuffer(1, fftSize),
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
    fftBuffer.setSize(1, fftSize);
}

void PluginProcessor::releaseResources()
{
    fftBuffer.clear();
    fundamentalFrequency.clear();
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
    auto numSamples = buffer.getNumSamples();
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);
    // ******************************************************************

    if (totalNumInputChannels == 2 && totalNumOutputChannels == 2)
    {
        // process for each channel
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto *channelData = buffer.getWritePointer(channel);

            // buffer FFT
            fftBuffer.clear();
            for (int i = 0; i < numSamples && i < fftSize; ++i)
            {
                fftBuffer.setSample(0, i, channelData[i]);
            }
            fft.performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(0));

            // find fundamental frequency
            float maxMagnitude = 0.0f;
            int maxIndex = 0;
            for (int i = 1; i < fftSize / 2; ++i)
            {
                float magnitude = fftBuffer.getSample(0, i);
                if (magnitude > maxMagnitude)
                {
                    maxMagnitude = magnitude;
                    maxIndex = i;
                }
            }
            fundamentalFrequency[channel] = static_cast<int>(static_cast<float>(maxIndex) * (getSampleRate() / static_cast<float>(fftSize)));

            // apply comb bandpass filter
            if (fundamentalFrequency[0] > 50 && fundamentalFrequency[1] > 50)
            {
                applyCombFilterWithHarmonics(buffer, fundamentalFrequency[channel], 20);
            }
        }
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

void PluginProcessor::applyBandpassFilter(juce::AudioBuffer<float> &buffer, float cutoffFrequency)
{
    std::vector<double> x1;
    std::vector<double> x2;
    std::vector<double> y1;
    std::vector<double> y2;

    x1.resize(buffer.getNumChannels(), 0.f);
    x2.resize(buffer.getNumChannels(), 0.f);
    y1.resize(buffer.getNumChannels(), 0.f);
    y2.resize(buffer.getNumChannels(), 0.f);

    // actual processing; each channel separately
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto channelSamples = buffer.getWritePointer(channel);

        const double tan = std::tan(PI * cutoffFrequency / getSampleRate());
        const double c = (tan - 1) / (tan + 1);
        const double d = -std::cos(2.f * PI * cutoffFrequency / getSampleRate());

        std::vector<double> b = {-c, d * (1.f - c), 1.f};
        std::vector<double> a = {1.f, d * (1.f - c), -c}; // resize the allpass buffers to the number of channels

        // for each sample in the channel
        for (auto i = 0; i < buffer.getNumSamples(); ++i)
        {
            double x = channelSamples[i];
            double y = b[0] * x + b[1] * x1[channel] + b[2] * x2[channel] - a[1] * y1[channel] - a[2] * y2[channel];

            y2[channel] = y1[channel];
            y1[channel] = y;
            x2[channel] = x1[channel];
            x1[channel] = x;

            // scale by 0.5 to stay in the [-1, 1] range
            channelSamples[i] = 0.5f * static_cast<float>(x - y);
        }
    }
}

void PluginProcessor::applyCombFilterWithHarmonics(juce::AudioBuffer<float> &buffer, int fundamentalFreq, int numHarmonics)
{
    const double sampleRate = getSampleRate();
    const float scalingFactor = 0.5f; // Scale to avoid clipping if many harmonics are added

    std::vector<std::vector<double>> x1, x2, y1, y2;
    x1.resize(numHarmonics, std::vector<double>(buffer.getNumChannels(), 0.0));
    x2.resize(numHarmonics, std::vector<double>(buffer.getNumChannels(), 0.0));
    y1.resize(numHarmonics, std::vector<double>(buffer.getNumChannels(), 0.0));
    y2.resize(numHarmonics, std::vector<double>(buffer.getNumChannels(), 0.0));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto *channelSamples = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float combinedOutput = 0.0f;

            for (int n = 1; n <= numHarmonics; ++n)
            {
                double harmonicFrequency = n * fundamentalFreq;
                if (harmonicFrequency >= sampleRate / 2.0)
                    break; // Nyquist limit

                double omega = 2.0 * PI * harmonicFrequency / sampleRate;
                double tanVal = std::tan(omega);
                double c = (tanVal - 1.0) / (tanVal + 1.0);
                double d = -std::cos(omega * 2.0);

                std::vector<double> b = {-c, d * (1.0 - c), 1.0};
                std::vector<double> a = {1.0, d * (1.0 - c), -c};

                double x = static_cast<double>(channelSamples[i]);
                double y = b[0] * x + b[1] * x1[n - 1][channel] + b[2] * x2[n - 1][channel] - a[1] * y1[n - 1][channel] - a[2] * y2[n - 1][channel];

                // Update delay buffers
                x2[n - 1][channel] = x1[n - 1][channel];
                x1[n - 1][channel] = x;
                y2[n - 1][channel] = y1[n - 1][channel];
                y1[n - 1][channel] = y;

                // Sum the harmonic component to the combined output
                combinedOutput += static_cast<float>(y);
            }

            // Apply scaling and assign to buffer
            channelSamples[i] = scalingFactor * combinedOutput;
        }
    }
}

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
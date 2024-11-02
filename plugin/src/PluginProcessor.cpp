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

    // process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto *channelData = buffer.getWritePointer(channel);
        // FFT with window
        fftBuffer.clear();
        for (int i = 0; i < fftSize && i < numSamples; ++i)
        {
            float windowedSample = channelData[i] * 0.5f * (1.0f - cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
            fftBuffer.setSample(0, i, windowedSample);
        }
        fft.performRealOnlyForwardTransform(fftBuffer.getWritePointer(0), true);

        // find fundamental frequency (F0)
        float maxMagnitude = 0.0f;
        int fundamentalBin = 0;
        for (int i = 1; i < fftSize / 2; ++i)
        {
            float magnitude = fftBuffer.getSample(0, i);
            if (magnitude > maxMagnitude)
            {
                maxMagnitude = magnitude;
                fundamentalBin = i;
            }
        }
        // calculate F0 frequency in Hz
        fundamentalFrequency[channel] = static_cast<int>(fundamentalBin * (getSampleRate() / fftSize)) / 2;
        DBG(fundamentalFrequency[channel]);

        // WORKS!!!!

        // identify harmonic bins
        std::vector<int> harmonicBins;
        for (int harmonicIndex = 1; harmonicIndex < 4; ++harmonicIndex)
        {
            int harmonicFrequency = fundamentalFrequency[channel] * harmonicIndex;
            int harmonicBin = static_cast<int>(harmonicFrequency * fftSize / getSampleRate());
            harmonicBins.push_back(harmonicBin);
        }
        // DBG("fft size is: " + (juce::String)fftSize + " harmonic 1: " + (juce::String)harmonicBins[0] + " 2: " + (juce::String)harmonicBins[1] + " 3: " + (juce::String)harmonicBins[2]);

        // apply comb filtering
        int bandWidth = 3;
        for (int i = 0; i < fftSize / 2; ++i)
        {
            bool isInHarmonicRange = false;

            for (int harmonicBin : harmonicBins)
            {
                if (i >= harmonicBin - bandWidth && i <= harmonicBin + bandWidth)
                {
                    isInHarmonicRange = true;
                    break;
                }
            }

            if (!isInHarmonicRange)
            {
                fftBuffer.setSample(0, i, 0.0f); // Zero out non-harmonic frequencies
            }
        }

        // IFFT and send to output buffer
        fft.performRealOnlyInverseTransform(fftBuffer.getWritePointer(0));
        buffer.copyFrom(channel, 0, fftBuffer, 0, 0, numSamples);
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

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
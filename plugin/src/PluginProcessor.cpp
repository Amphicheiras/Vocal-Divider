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
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    // ******************************************************************

    // prepare voice buffers
    auto numSamples = buffer.getNumSamples();
    juce::AudioBuffer<float> leftVoiceBuffer(1, numSamples);
    juce::AudioBuffer<float> rightVoiceBuffer(1, numSamples);
    auto *leftChannelInput = buffer.getWritePointer(0);
    auto *rightChannelInput = buffer.getWritePointer(1);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // do FFT
        fftBuffer.clear();
        for (int i = 0; i < numSamples && i < fftSize; ++i)
        {
            fftBuffer.setSample(0, i, leftChannelInput[i]);
        }
        fft.performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(0));

        // find fundamental frequency (F0)
        float maxMagnitude = 0.0f;
        int maxIndex = 0;
        for (int i = 1; i < fftSize / 2; ++i)
        {
            if (fftBuffer.getSample(0, i) > maxMagnitude)
            {
                maxMagnitude = fftBuffer.getSample(0, i);
                maxIndex = i;
            }
        }

        // calculate F0
        fundamentalFrequency[0] = static_cast<int>(static_cast<float>(maxIndex) * (getSampleRate() / static_cast<float>(fftSize)));

        // do FFT
        fftBuffer.clear();
        for (int i = 0; i < numSamples && i < fftSize; ++i)
        {
            fftBuffer.setSample(0, i, rightChannelInput[i]);
        }
        fft.performFrequencyOnlyForwardTransform(fftBuffer.getWritePointer(0));

        // find fundamental frequency (F0)
        maxMagnitude = 0.0f;
        maxIndex = 0;
        for (int i = 1; i < fftSize / 2; ++i)
        {
            if (fftBuffer.getSample(0, i) > maxMagnitude)
            {
                maxMagnitude = fftBuffer.getSample(0, i);
                maxIndex = i;
            }
        }

        // calculate F0
        fundamentalFrequency[1] = static_cast<int>(static_cast<float>(maxIndex) * (getSampleRate() / static_cast<float>(fftSize)));

        // identify harmonic bins
        std::vector<int> harmonicBins;
        for (int harmonicIndex = 1; harmonicIndex < 4; ++harmonicIndex)
        {
            int harmonicFrequency = fundamentalFrequency[channel] * harmonicIndex;
            int harmonicBin = static_cast<int>(harmonicFrequency * fftSize / getSampleRate());
            harmonicBins.push_back(harmonicBin);
        }
        DBG("1: " + (juce::String)harmonicBins[0] + " 2: " + (juce::String)harmonicBins[1] + " 3: " + (juce::String)harmonicBins[2]);

        // set a band around each harmonic bin and zero out the rest
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

        // Perform inverse FFT to return to time domain
        fft.performRealOnlyInverseTransform(fftBuffer.getWritePointer(0));

        // Copy filtered data back to the output buffer
        for (int i = 0; i < numSamples && i < fftSize; ++i)
        {
            if (channel == 0)
                rightVoiceBuffer.setSample(0, i, fftBuffer.getSample(0, i));
            else
                leftVoiceBuffer.setSample(0, i, fftBuffer.getSample(0, i));
        }
    }

    buffer.copyFrom(0, 0, leftVoiceBuffer, 0, 0, numSamples);
    buffer.copyFrom(1, 0, rightVoiceBuffer, 0, 0, numSamples);
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
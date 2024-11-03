#include "VocalDivider/PluginProcessor.h"
#include "VocalDivider/PluginEditor.h"

PluginProcessor::PluginProcessor() : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                                     fft(fftOrder),
                                     fftBuffer(2, fftSize),
                                     fundamentalFrequency(2, 0),
                                     bandpassFilter(juce::dsp::IIR::Coefficients<float>::makeBandPass(44100, 20000.0f, 0.1f))
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

    bandpassFilter.prepare(spec);
    bandpassFilter.reset();
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

    juce::dsp::AudioBlock<float> block(buffer);
    updateFilter();
    bandpassFilter.process(juce::dsp::ProcessContextReplacing<float>(block));

    // apply bandpass
    // if (fundamentalFrequency[0] > 0)
    //     applyMultiBandPassFilter(buffer, 0, fundamentalFrequency[0]);
    // if (fundamentalFrequency[1] > 0)
    //     applyMultiBandPassFilter(buffer, 1, fundamentalFrequency[1]);
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

// void PluginProcessor::applyBandpassFilter(juce::AudioBuffer<float> &buffer, int centerFrequency, int bandwidth)
// {

// }

// void PluginProcessor::applyMultiBandPassFilter(juce::AudioBuffer<float> &buffer, int channel, int f0)
// {

// }

void PluginProcessor::updateFilter()
{
    float freq = 500.f;
    float res = 3.f;

    *bandpassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(44100, freq, res);
}

void PluginProcessor::setCenterFrequency(float newFrequency)
{
    centerFrequency = newFrequency;
    updateFilter();
}

void PluginProcessor::setQFactor(float newQ)
{
    Q = newQ;
    updateFilter();
}
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

const std::string ReversedDelayAudioProcessor::paramsNames[] = { "Delay", "Feedback", "Mix", "Volume" };
//==============================================================================
ReversedCircularBuffer::ReversedCircularBuffer()
{
}

void ReversedCircularBuffer::init(int size)
{
	m_head = 0;
	m_headReversed = m_bufferSize = m_delaySize = size;

	m_buffer = NULL;
	m_buffer = new float[size];
	memset(m_buffer, 0.0f, size * sizeof(float));
}

void ReversedCircularBuffer::clear()
{
	m_head = 0;
	m_headReversed = m_delaySize = m_bufferSize;
	memset(m_buffer, 0.0f, sizeof(m_buffer));
}

//==============================================================================
ReversedDelayAudioProcessor::ReversedDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	delayParameter    = apvts.getRawParameterValue(paramsNames[0]);
	feedbackParameter = apvts.getRawParameterValue(paramsNames[1]);
	mixParameter      = apvts.getRawParameterValue(paramsNames[2]);
	volumeParameter   = apvts.getRawParameterValue(paramsNames[3]);
}

ReversedDelayAudioProcessor::~ReversedDelayAudioProcessor()
{
}

//==============================================================================
const juce::String ReversedDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReversedDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ReversedDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ReversedDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ReversedDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ReversedDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ReversedDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ReversedDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ReversedDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void ReversedDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ReversedDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	const int size = (int)(DELAY_MAX_MS * 0.001f * 2.0f *  sampleRate);
	m_reversedCircularBuffer[0].init(size);
	m_reversedCircularBuffer[1].init(size);

	m_reversedCircularBuffer[0].clear();
	m_reversedCircularBuffer[1].clear();

	// Update delay time
	float delay = delayParameter->load();

	const int delaySamples = (int)(delay * 0.001f * 2.0f * getSampleRate());
	m_reversedCircularBuffer[0].setSize(delaySamples);
	m_reversedCircularBuffer[1].setSize(delaySamples);
}

void ReversedDelayAudioProcessor::releaseResources()
{
	m_reversedCircularBuffer[0].clear();
	m_reversedCircularBuffer[1].clear();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ReversedDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ReversedDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	// Get params
	float delay = delayParameter->load();
	float feedback = feedbackParameter->load();
	const auto mix = mixParameter->load();
	float volume = juce::Decibels::decibelsToGain(volumeParameter->load());

	// Mics constants
	const float mixInverse = 1.0f - mix;
	const int channels = getTotalNumOutputChannels();
	const int samples = buffer.getNumSamples();
	const int delaySamples = (int)(delay * 0.001f * 2.0f * getSampleRate());

	for (int channel = 0; channel < channels; ++channel)
	{
		auto* channelBuffer = buffer.getWritePointer(channel);
		auto& reversedCircularBuffer = m_reversedCircularBuffer[channel];

		reversedCircularBuffer.setSize(delaySamples);
		reversedCircularBuffer.setFeedback(feedback);

		for (int sample = 0; sample < samples; ++sample)
		{
			// Get input
			const float in = channelBuffer[sample];

			//Delay
			reversedCircularBuffer.writeSample(in + feedback * reversedCircularBuffer.read());			
			const float reversedDelayOut = reversedCircularBuffer.readReversed();

			// Apply volume
			channelBuffer[sample] = volume * (mix * reversedDelayOut + mixInverse * in);
		}
	}
}

//==============================================================================
bool ReversedDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ReversedDelayAudioProcessor::createEditor()
{
    return new ReversedDelayAudioProcessorEditor(*this, apvts);
}

//==============================================================================
void ReversedDelayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	auto state = apvts.copyState();
	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void ReversedDelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if (xmlState.get() != nullptr)
		if (xmlState->hasTagName(apvts.state.getType()))
			apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout ReversedDelayAudioProcessor::createParameterLayout()
{
	APVTS::ParameterLayout layout;

	using namespace juce;

	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[0], paramsNames[0], NormalisableRange<float>( 50.0f, DELAY_MAX_MS,  1.0f, 1.0f), 100.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[1], paramsNames[1], NormalisableRange<float>(  0.0f,         1.0f, 0.05f, 1.0f),   0.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[2], paramsNames[2], NormalisableRange<float>(  0.0f,         1.0f, 0.05f, 1.0f),   1.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[3], paramsNames[3], NormalisableRange<float>(-12.0f,        12.0f,  0.1f, 1.0f),   0.0f));

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReversedDelayAudioProcessor();
}

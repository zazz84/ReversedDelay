/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class ReversedCircularBuffer
{
public:
	ReversedCircularBuffer();

	void init(int size);
	void clear();
	void writeSample(float sample)
	{
		m_buffer[m_head] = m_coefBuffer * m_buffer[m_head] + m_coefIn * sample;
		if (++m_head >= m_delaySize)
			m_head -= m_delaySize;
	}
	float read()
	{
		return m_buffer[m_head];
	}
	float readReversed()
	{
		if (--m_headReversed < 0)
			m_headReversed += m_delaySize;

		// Apply fade in/out
		/*const float threshold = m_delaySize * 0.1f;

		if (m_headReversed < threshold)
		{
			const float factor = m_headReversed / threshold;
			return factor * m_buffer[m_headReversed];
		}
		else if (m_delaySize - threshold < m_headReversed)
		{
			const float factor = (m_delaySize - m_headReversed) / threshold;
			return factor * m_buffer[m_headReversed];
		}
		else if (abs(m_headReversed - m_delaySize * 0.5f) < threshold)
		{
			const float factor = abs(m_headReversed - m_delaySize * 0.5f) / threshold;
			return factor * m_buffer[m_headReversed];
		}
		else
		{
			return m_buffer[m_headReversed];
		}*/

		return m_buffer[m_headReversed];
	}
	void setSize(int size)
	{
		m_delaySize = (int)fminf(m_bufferSize, size);
	}
	void setFeedback(float coef)
	{
		float coefOptimized = coef * 0.5f;

		m_coefBuffer = 0.5f + coefOptimized;
		m_coefIn = 1.0f - m_coefBuffer;
	}

protected:
	float *m_buffer;
	float m_coefBuffer = 0.0f;
	float m_coefIn = 1.0f;
	int m_head = 0;
	int m_headReversed = 0;
	int m_delaySize = 0;
	int m_bufferSize = 0;
};

//==============================================================================
/**
*/
class ReversedDelayAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    ReversedDelayAudioProcessor();
    ~ReversedDelayAudioProcessor() override;

	static const std::string paramsNames[];
	static const int DELAY_MAX_MS = 500;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	using APVTS = juce::AudioProcessorValueTreeState;
	static APVTS::ParameterLayout createParameterLayout();

	APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:
    //==============================================================================

	std::atomic<float>* delayParameter = nullptr;
	std::atomic<float>* feedbackParameter = nullptr;
	std::atomic<float>* mixParameter = nullptr;
	std::atomic<float>* volumeParameter = nullptr;

	ReversedCircularBuffer m_reversedCircularBuffer[2] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReversedDelayAudioProcessor)
};

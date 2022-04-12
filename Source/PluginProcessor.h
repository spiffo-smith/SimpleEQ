/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

// This is a structure that the GUI thread can use to retrieve the blocks from the 'SingleChannelSampleFifo' structure
// this is all explained in the video for Project 10 which is part of the programmingformusicians.com course
//----------------------------------------------------------------------------------------------------------
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v < T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");

        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,       // clear everything
                           true,        // including the extra space?
                           true);       // avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v < T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");

        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        return false;
    }
    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }
        return false;
    }
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

// declares an enum for use in our Spectrum Analyser
//--------------------------------------------------
enum Channel
{
    Right,  // effectively 0
    Left    // effectively 1
};

// The Juce FFT Algorithm works on a fixed number of samples, the host passes in buffers of a variety of different sizes
// we need a way of collecting individual samples into blocks of fixed sizes and this is what this structure does!
// the exact way it works is explained in the 'Project 11' Video which is part of the programmingformusicians.com course
//----------------------------------------------------------------------------------------------------------------------
template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
    prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }
    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,         // channel
                             bufferSize,// num samples
                             false,     // keep existing content
                             true,      // clear extra space
                             true);     // avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //============================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //============================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);
            juce::ignoreUnused(ok);
            fifoIndex = 0;
        }
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

// declares an enum called 'Slope' for use in our Cut Filters, Slope_12 = 0, Slope_24 = 1, etc...
// enums are a way of having a restricted group of values for a variable, by default their integer value is 0,1,2,3 . .etc
//------------------------------------------------------------------------------------------------------------------------
enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// creates a Structure called 'ChainSettings', structs are very similar to classes, structs are public by default
// the lowCutSlope & highCutSlope get assigned initial values from the 'Slope' enum
//---------------------------------------------------------------------------------
struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };

    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };

    bool lowCutBypassed{ false }, peakBypassed{ false }, highCutBypassed{ false };
    // 3 booleans to represent the Bypass Buttons State
};

// declares a getChainSettings function that passes a reference to apvts into it?
// the definition of what it does is in the .cpp file
// --------------------------------------------------
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;
// Filter is now an 'Alias', it saves us time from typing out the long form!

using CutFilter = juce::dsp::ProcessorChain <Filter, Filter, Filter, Filter>;
// CutFilter is now an 'Alias'. It is a 48dB slope filter as it uses the Juce 'ProcessorChain' to put 4 Filters in series!

using MonoChain = juce::dsp::ProcessorChain <CutFilter, Filter, CutFilter>;
// MonoChain is now an 'Alias'. This represents 1 Channel of our Stereo Signal Path through this SimpleEQ Audio Processor
// The Signal Path is a Low Cut followed by some other type of filter (Band Pass, Q, etc) then a High Pass at the end

// we declare an enum (enumerator) called 'chainPositions'
// ------------------------------------------------------
enum ChainPositions {
    LowCut, Peak, HighCut
};

using Coefficients = Filter::CoefficientsPtr;
// Coefficients is now a Alias to Filter::CoefficientsPtr

void updateCoefficients(Coefficients& old, Coefficients& replacements);
// declares a helper function for updating Filter Coefficients

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);
// declares a function 'makePeakFilter' which will contruct a Peak Filter

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}
// Above is a helper function with a template for updating Cut Filters. The Index, ChainType and cutCoefficients are passed in
// the old coefficients and the new coefficients are passed into the updateCoefficients function and the indexed filter is unbypassed

template<typename ChainType, typename CoefficientType, typename Slope>
void updateCutFilter(ChainType& leftLowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope)
{
    leftLowCut.template setBypassed<0>(true);
    leftLowCut.template setBypassed<1>(true);
    leftLowCut.template setBypassed<2>(true);
    leftLowCut.template setBypassed<3>(true);
    // start by bypassing all 4 filters in the chain. The .template part tells it to use the template

    switch (lowCutSlope)
    {
    case Slope_48:
    {
        update<3>(leftLowCut, cutCoefficients);
    }
    case Slope_36:
    {
        update<2>(leftLowCut, cutCoefficients);
    }
    case Slope_24:
    {
        update<1>(leftLowCut, cutCoefficients);
    }
    case Slope_12:
    {
        update<0>(leftLowCut, cutCoefficients);
    }
    }
    // Switch Case Statement. Depending on the value from the lowCutSlope we have either 1, 2, 3 or 4 Filers (12dB) active
    // the index no. the ChainType & the new coefficients are passed into the helper function called 'update'
}

// helper function for making the Low Cut Filter
//----------------------------------------------
inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                        sampleRate,
                                                                                        2 * (chainSettings.lowCutSlope + 1));
   // returns our low cut filter coefficients from our parameters using the High Pass High Order Butterworth Filter Method
   // the last value passed in is the Filter Order, i.e. lowCutslope = 3 so Order = 8 (this is 48dB Slope)
   // depending on the order value passed in, the function creates multiple 'cutCoefficients' in an array
   // if you CTRL Click into the 'designIIRHighpassHighOrderButterworthMethod' you can see how this Order no. is used
}

// helper function for making the High Cut Filter
//-----------------------------------------------
inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                       sampleRate,
                                                                                       2 * (chainSettings.highCutSlope + 1));
}

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

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

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    // declare the function for creating our Parameter Layout, static means it cannot be modified?

    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };
        // declare our AudioProcessorValueTreeState. It needs to be in the public section
        // *this is this Audio Processor, nullptr because we aren't using an Undo method
        // "Parameters" is the valueTreeType
        // createParameterLayout() function is how we create the ParameterLayout, the function is in PluginProcessor.cpp

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo { Channel::Left };
    SingleChannelSampleFifo<BlockType> rightChannelFifo { Channel::Right };
    // declares our 2 Fifo Buffer Structures called 'leftChannelFifo' & 'rightChannelFifo'

private:

    MonoChain leftChain, rightChain;
    // we declare variables called leftChain & rightChain which are derived from MonoChain

    void updatePeakFilter(const ChainSettings& chainSettings);
    // declares a function for updating the Peak Filter Coefficients
    // takes in a parameter called 'chainSettings' which is a reference to the 'ChainSettings' Structure

    

    void updateLowCutFilters(const ChainSettings& chainSettings);

    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();

    juce::dsp::Oscillator<float> osc;
    // declares a Test Oscillator

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};

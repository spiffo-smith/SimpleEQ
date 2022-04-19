/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// enumerator for use in our FFT Data Generator
// i.e. order2048 will be used to slice up the Frequency Range (20Hz - 20kHz) into 2048 equally sized chunks
//----------------------------------------------------------------------------------------------------------
enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

// This is the FFT Data Generator all the explanation for this is in the PFM Project 11 Course
//--------------------------------------------------------------------------------------------
template<typename BlockType>
struct FFTDataGenerator
{
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
  
        window->multiplyWithWindowingTable(fftData.data(), fftSize);
        // first apply a windowing function to our data

        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());
        // then render our FFT Data

        int numBins = (int)fftSize / 2;

        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] /= (float)numBins;
        }
        // normalize the FFT values

        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        // convert them to Decibels

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        // when you change order, recreate the window, forwardFFT, fifo, fftData
        // also reset the fifoIndex
        // things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

// Declares a structure for building a path from the FFT Data, simplified version of the one in PFM Project 11
// it has various functions in it for generating the path and returning the data
//------------------------------------------------------------------------------
template<typename PathType>
struct AnalyserPathGenerator
{
    // converts 'renderData[]' into a juce::path
    void generatePath(const std::vector<float>& renderData,
                    juce::Rectangle<float> fftBounds,
                    int fftSize,
                    float binWidth,
                    float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom), top);
        };
        
        auto y = map(renderData[0]);

        jassert(!std::isnan(y) && !std::isinf(y));

        p.startNewSubPath(0, y);

        const int pathResolution = 2; // you can draw line-to's every 'pathResolution' pixels

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            jassert(!std::isnan(y) && !std::isinf(y));

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }
        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

// Declares a Look And Feel (version 4) Structure for building a 'drawRotarySlider'
//---------------------------------------------------------------------------------
struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;
};

// Declare a Structure that will define & build all our Rotary GUI Controls
//-------------------------------------------------------------------------
struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                     juce::Slider::TextEntryBoxPosition::NoTextBox),
        param (&rap),
        suffix (unitSuffix)
    {
        setLookAndFeel(&lnf);
        // calls setLookAndFeel and passes in the Memory Address of 'lnf' which is the LookAndFeel Instance
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }
    // Destructor that removes the Look And Feel

    struct LabelPos
    {
        float pos;
        juce::String label;
    };
    // Structure that creates a floating point 'pos' and a string called 'label'

    juce::Array<LabelPos> labels;
    // declares an Array of 'LabelPos' Structures called 'labels'

    void paint(juce::Graphics& g) override;
    // this is our function declaration for painting our Sliders
    // override means this function overrides any other function called the same name

    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }    // just returns 12 if the getTextHeight function is called
    juce::String getDisplayString() const;

private:
    LookAndFeel lnf;
    // defines a Look and Feel instance called 'lnf'
    juce::RangedAudioParameter* param;
    juce::String suffix;
    // defines an instance of RangedAudioParameter and String
};

// Structure for Producing FFT Analyser Paths
// ------------------------------------------
struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>& scsf) : leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
        // sets the FFT Data Generator to 2048 chunks over 20Hz to 20kHz & sets the monoBuffer to the correct size
    }
    void process(juce::Rectangle<float> fftBounds, double samplRate);
    juce::Path getPath() { return leftChannelFFTPath; }

private:
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* leftChannelFifo;
    // declares a pointer to a SingleChannelSampleFifo called 'leftChannelFifo'

    juce::AudioBuffer<float> monoBuffer;
    // declares a buffer called 'monoBuffer' for use in our FFT Analysis

    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    // declares our FFT Data Generator

    AnalyserPathGenerator<juce::Path> pathProducer;
    // add an instance of our FFT Analyser Path Generator

    juce::Path leftChannelFFTPath;
    // add an instance of our FFT Analysis Path
};

// declares a Structure called ResponseCurveComponent which is a separate 'GUI Component' for the Response Curve
//--------------------------------------------------------------------------------------------------------------
struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
    // add this line inside the Response Curve Structure so it can become a listener to the parameters
    // we also need the timer to periodically check if parameters have changed
{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    // function we will use this to query whether our parameters have changed

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    // we aren't using this

    void timerCallback() override;
    // declares a timerCallback function which we will use to query an Atomic Flag to see if parameters have changed

    void paint(juce::Graphics& g) override;
    // declares a paint function

    void resized() override;
    // declares a function called 'resized'

private:
    SimpleEQAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };
    // declares an Atomic Timer called 'parametersChanged' initially set to false

    MonoChain monoChain;
    // adds an instance of the Mono Chain for use in displaying the Response Curve in real time!

    void updateChain();
    // declares an updateChain function

    juce::Image background;
    // declares an image inside the ResponseCurveComponent called 'background'

    juce::Rectangle<int> getRenderArea();
    // declares a function called 'getRenderArea'

    juce::Rectangle<int> getAnalysisArea();
    // declares a a function called 'getAnalysisArea' that we draw our Response Curve within

    PathProducer leftPathProducer, rightPathProducer;
    // declares instances of left & right FFT Path Producers built from the PathProducer Structure

};

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor

{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    RotarySliderWithLabels peakFreqSlider,
                           peakGainSlider, 
                           peakQualitySlider,
                           lowCutFreqSlider,
                           highCutFreqSlider,
                           lowCutSlopeSlider,
                           highCutSlopeSlider;
    // declares all our GUI Sliders, created from the RotarySliderWithLabels Structure defined at the top of the file

    ResponseCurveComponent responseCurveComponent;
    // adds an instance of the Response Curve

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    // create an Alias called 'APVTS', then another Alias called 'Attachment' so we can shorten the otherwise very verbose code

    Attachment peakFreqSliderAttachment,
        peakGainSliderAttachment,
        peakQualitySliderAttachment,
        lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment,
        highCutSlopeSliderAttachment;
    // now we can declare the attachments using the Aliases defined above

    juce::ToggleButton lowcutBypassButton, peakBypassButton, highcutBypassButton, analyzerEnabledButton;
    // add some standard juce toggle buttons for the Bypass Buttons

   using ButtonAttachment = APVTS::ButtonAttachment;
   ButtonAttachment lowcutBypassButtonAttachment,
                   peakBypassButtonAttachment,
                   highcutBypassButtonAttachment,
                   analyzerEnabledButtonAttachment;
    // declares our Bypass Button Attachments, we need to connect them to parameters in the PluginEditor Constructor

    std::vector<juce::Component*> getComps();
    // declares a function called 'getComps' which is a vector of pointers

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};

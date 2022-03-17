/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;
};
// Declares a Look And Feel (version 4) Structure for building a 'drawRotarySlider'

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

    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 10; }    // just returns 12 if the getTextHeight function is called
    juce::String getDisplayString() const;

private:
    LookAndFeel lnf;
    // defines a Look and Feel instance called 'lnf'
    juce::RangedAudioParameter* param;
    juce::String suffix;
    // defines an instance of RangedAudioParameter and String
};
// Declare a Structure that will define & build all our Rotary GUI Controls

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

private:
    SimpleEQAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };
    // declares an Atomic Timer called 'parametersChanged' initially set to false

    MonoChain monoChain;
    // adds an instance of the Mono Chain for use in displaying the Response Curve in real time!
};
// declares a separate 'GUI Component' for the Response Curve

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

    std::vector<juce::Component*> getComps();
    // declares a function called 'getComps' which is a vector of pointers

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colour(97u, 18u, 167u));
    g.fillEllipse(bounds);
    // draws a filled in purple ellipse

    g.setColour(Colour(255u, 154u, 1u));
    g.drawEllipse(bounds, 1.f);
    // draws an orange ellipse line around the outside

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
        // creates a variable rswl which is cast of RotarySliderWithLabels, if that works then do all the stuff below
    {
        auto center = bounds.getCentre();
        // gets the centre coordinates of the rectangle

        Path p;
        // if we want to rotate something on the GUI we need to define it inside a Juce Path
        // 'p' is the path we will rotate inside our slider to indicate it's position

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        // defines a Rectangle 'r' which will be our graphical line inside our path 'p' that indicates the slider's current position
        // sets it's left edge minus 2 pixels from the 'center', and right edge plus 2 pixels
        // it's top to the top of the bounds and it's bottom to the centre of the 'bounds' rectangle minus 1.5 times the Text Height
       
        p.addRoundedRectangle(r, 2.f);
        // adds the rounded rectangle 'r' to the path 'p'

        jassert(rotaryStartAngle < rotaryEndAngle);
        // this is a check that the start angle passed in is less than the end angle, I'm not clear why we need it?

        auto rotaryAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        // the 'rotaryAngRad' is made equal to the converted into radians normalised angle between 0 and 1

        p.applyTransform(AffineTransform().rotated(rotaryAngRad, center.getX(), center.getY()));
        // this applies a rotary transformation to the path 'p' by the angle in radians calculated above, at the center point

        g.fillPath(p);
        // this draws the path 'p' inside our rotary slider

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        // setting up the display text variables

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        // creating a rectangle around the displayed text

        g.setColour(Colours::black);
        g.fillRect(r);
        // fill that rectangle with black

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
        // draw in white the 'text' inside the rectangle, centred and 1 line of text
    }
}
// defines the LookAndFeel::drawRotarySlider function

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    // these set the Start and End Stops of the Rotary Sliders movement
    // the 'MathConstants<float>::twoPi' adds a full 360 rotation, I'm not entirely sure why we need it?

    auto range = getRange();
    auto sliderBounds = getSliderBounds();

    // g.setColour(Colours::red);
    // g.drawRect(getLocalBounds());
    // g.setColour(Colours::yellow);
    // g.drawRect(sliderBounds);
    // draws some coloured rectangles around different bounds so we can see them for debugging purposes

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        // jmap turns our slider's value into a normalised value between 0 and 1 for display purposes
        startAng,
        endAng,
        *this);
    // this section above is what draws our Slider

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    // we loop around our Array called 'labels', each entry has a 'pos' and a string 'label'
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        // this extracts the 'pos' from the array, calculates it's angle, then calculates a display center point 'c'

        auto str = labels[i].label;

        Rectangle<float> r;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        // this creates a rectangle, & sets it's centre to 'c' which we calculated above

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
        // this draws & fits the text from the array identifier 'label' inside the rectangle 'r'
    }
    // the code above is what draws our min & max 'labels' onto their correct position
}
// this is our paint function for drawing a 'RotarySliderWithLabels' using the JUCE LookAndFeel::drawRotarySlider

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    // this finds the minimum of width and height and makes variable 'size' equal to that

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    // creates a rectangle which is a square of 'size' minus 2 times the text height
    // it's X centre is the X centre of the local bounds and it's 2 pixels down from the top

    return r;
    // returns the rectangle 'r' when the getSliderBounds function is called
}
// this defines our getSliderBounds function, it returns a juce Rectangle

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();
    // if you can cast the parameter from a juce AudioParameterChoice then return the Choice Name
    // this returns the 12dB/Oct, 24dB/Oct etc....

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        // if you can cast the parameter into a floating point then do the code below
    {
        float val = getValue();
        if (val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }
        // if the value of the parameter is greater than 999 then it must be 1000 or over so make addK = true

        str = juce::String(val, (addK ? 2 : 0));
        // make str equal to 'val', if addK is true then restrict it to 2 decimal places otherwise use how many you need (that's what 0 means)
    }
    else
    {
        jassertfalse; // this shouldn't happen
    }

    if (suffix.isNotEmpty())
        // if suffix is not the empty one we put in Peak Quality Slider then do the code below
    {
        str << " ";
        // add a space
        if (addK)
            str << "k";
            // if addK is equal to true then add a 'k' suffix
 
        str << suffix;
        // add the suffix that was defined for this parameter
    }
    return str;
}
// function that gets the Slider's current value and or Audio Prameter Choice & returns it as a string for display inside the Slider

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    // creates an array of pointers that are returned by the getParameters function

    for (auto param : params)
    {
        param->addListener(this);
    }
    // loops around the vector 'params' and adds the Editor as a Listener for each one

    updateChain();
    // calls the updateChain function
    startTimerHz(60);
    // starts a 60Hz ticking timer!
}
// Constrcutor for the ResponseCurveComponent

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    // creates an array of pointers that are returned by the getParameters function

    for (auto param : params)
    {
        param->removeListener(this);
    }
    // loops around the vector 'params' and removes the Response Curve Component as a Listener for each one
    // this is important to add in the De-Constructor, not sure why?
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
    // sets our Atomic Clock to true
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
        // if 'parametersChanged is true set to false and run code below
    {
        DBG("params changed");
        // writes "params changed" to the standard error stream
        updateChain();
        // calls the updateChain function
        repaint();
        // signal a repaint
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    // get the chain settings from the apvts
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    // get the peakCoefficients
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    // get the low cut coefficients
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    // get the high cut coefficients

    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    // update the monoChain
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // stops us having to type juce:: in front of everything

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    auto responseArea = getLocalBounds();
    // sets the response area where we draw our EQ Curve equal to the top third of the GUI

    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;
    // declares a vector to store all the double numbers coming from the FFT analysis

    mags.resize(w);
    // resizes the vector to be 1 mag per pixel of the response area width?

    for (int i = 0; i < w; i++)
    {
        double mag = 1.f;
        // this gives us a starting gain of 1
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        // converts the pixel position into a frequency using a logarithmic function which matches the human hearing

        if (!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // if it's NOT bypassed, compute the magnitude for that frequency and multiply 'mag' by it

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // the low cut filter is acutally 4 filters in series so we need to test and compute each one

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
        // converts the final computed gain from all the filters to decibels and stores it in the mags vector!
    }
    // loops around once for each pixel of width (w) and calculates the gain at that point

    Path responseCurve;
    // definies a 'Path' which is from the JUCE Class, it's a series of lines or curves you can draw to

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    // this finds the top and bottom of the response area

    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
        // function that re-maps the 'input', source range -24 to 24, to the responseArea limits
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    // starts a new sub path on our 'Path' starting at the left edge of the responseArea
    // we set it to what is returned from the 'map' function when we pass in the front of the 'mags' vector

    for (size_t i = 1; i < mags.size(); i++)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
        // this adds a line to 'responseCurve' for each x coordinate of the responseArea
    }
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    // draws an orange rectangle around the responseArea

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    // this draws the responseCurve
}

SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),

    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    // these define our Sliders and their Look And Feel, they have a name and a suffix

    responseCurveComponent(audioProcessor),

    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
    // these define our Slider Attachments that were declared in the pluginEditor.h
{
    peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20kHz" });
    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "+24dB" });
    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });
    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });
    lowCutSlopeSlider.labels.add({ 0.f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });
    highCutSlopeSlider.labels.add({ 0.f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });
    // these add our min & max labels to the Array called 'labels'

    for (auto* comp : getComps())
    {
    addAndMakeVisible(comp);
    }
    // for loop that calls the 'getComps' function, and adds and makes visible each component as it is returned from the vector

    setSize (600, 480);
    // sets the overall size of the Plugin Window
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{

}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // stops us having to type juce:: in front of everything
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    float hRatio = 25.f / 100.f; // JUCE_LIVE_CONSTANT(33) / 100.f;
    // hRatio is 0.25, if we delete '25.f / 100.f' and uncomment the JUCE_LIVE_CONSTANT we can dynamically change hRatio whilst the Plugin is running
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);
    // we are making 'bounds' equal to the whole GUI, then we are making 'responseArea' equal to it's total height * hRatio

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);
    // removes 5 pixels from the top of bounds after we have already removed the responseArea from the top
    // this puts a 5 pixel gap between bottom of responseArea and top of Sliders so it looks nicer

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    // we are making 'lowCutArea' equal to the left third, then 'highCutArea' the right third (half of the remaining)

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(bounds.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    // makes the 'lowCutFreqSlider' take up the top half of 'lowCutArea', and 'lowCutSlopeSlider takes the bottom half (what's left)

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(bounds.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    // makes the 'highCutFreqSlider' take up the top half of 'highCutArea', and 'highCutSlopeSlider takes the bottom half (what's left)

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    // at this point 'bounds' only comprises the bottom middle strip
    // 'peakFreqSlider' takes the top third, 'peakGainSlider' takes the middle third & 'peakQualitySlider' is left with the bottom third
}

std::vector <juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}
// 'getComps' function that creates a vector of the physical memory addresses of the GUI Components then returns them all in turn

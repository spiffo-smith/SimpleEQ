/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
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
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    // This sets up a 'spec' for our Audio Processing

    leftChain.prepare(spec);
    rightChain.prepare(spec);
    // This sets up our Left & Right Channels with the correct 'spec'

    updateFilters();
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();
    // calls the updateFilters function

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

   // Above is the buffer and sample block replacing code
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor (*this);
    // comment the line above out and un-comment the line below to use the JUCE Generic Editor GUI

    //return new juce::GenericAudioProcessorEditor(*this);
    // comment out the 1st line & un-comment the line above to use the JUCE Generic Editor GUI
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);

    // 1st line creates a memory block called 'mos' with the 'destData' passed intot he function
    // 2nd line writes the current apvts into the memory block 'mos'
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
    // creates a variable called 'tree' from the 'data' passed into the function
    // only is it passes the isValid test does the apvts get replaced with it and the filter coefficients updated
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {

    ChainSettings settings;
    
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    return settings;
}
// The getChainSettings function creates a ChainSettings Structure called 'settings' & loads up all the raw parameter values into it
// The lowCutSlope & highCutSlope have to be cast first from 0-3 into Slope_12, Slope_24, etc....

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
        // returns the Peak Filter Coefficients from our new parameters
        // the last 'gain' setting needs to be converted from our decibel figure
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    // makes the peakCoefficients by calling the makePeakFilter function above

    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
   // calls the 'updateCoefficients' function, passes in the old and the new coeffcients
}
// Above is the function for updating the Peak Filter coefficients

void updateCoefficients(Coefficients& old, Coefficients& replacements)
{
    *old = *replacements;
}
// function for updating filter coefficients, references to the Coefficients are passed in as old and replacements

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    // calls the helper function 'makeLowCutFilter' defined in PluginProcessor.h to return our Low Cut Filter Coefficients

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    // creates a variable reference called 'leftLowCut' which is the 'leftChain' 1st position (position 0 from the chainPositions enum)
    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    // calls the update Cut Filter function, passing in the left chain

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
    // does the same stuff for the right chain
}

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);

    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
    // above is all the same code but for the High Cut Filter
}

void SimpleEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updatePeakFilter(chainSettings);
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
    // calls the getChainSettings function which pulls in all the current filter parameters
    // then calls the functions to update all the filters
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                            "LowCut Freq",
                                                            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                            20.f));
    // Above is the definition of our createParameterLayout function, inside the function we call it 'layout'
    // add a parameter called LowCut Freq to 'layout' we give it some range values
    // start at 20Hz, finish at 20Khz, steps of 1, skew of 1, start at 20Hz

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                            "HighCut Freq",
                                                             juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                             20000.f));
  
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                             "Peak Freq",
                                                             juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                             750.f));
 
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                            "Peak Gain",
                                                            juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                            0.0f));
 
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                            "Peak Quality",
                                                            juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                            1.f));
    juce::StringArray stringArray;
    for (int i = 0; i < 4; i++) {
        juce::String str;
        str << (12 + i * 12);
        str << " dB/Oct";
        stringArray.add (str);
    }
    // Above we create a juce StringArray called 'stringArray'
    // we then loop around 4 times adding 12 dB/Oct, then 24 dB/Oct....etc into the stringArray

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    // now we have created our stringArray we can use it when we create the 2 AudioParameterChoice Parameters

    return layout;

    // now we have created our 'layout' return it to the Audio Processor Value Tree State Constructor
    // which called it in the PluginProcessor.h file
}

// 

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}

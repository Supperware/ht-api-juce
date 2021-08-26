#pragma once

#include <JuceHeader.h>
using namespace juce;

#include "HeadMatrix.h"
#include "midi.h"
#include "configPanel.h"
#include "headPanel.h"

//==============================================================================

class MainComponent  : public juce::Component, HeadPanel::HeadPanel::Receiver
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void trackerChanged(const HeadMatrix& headMatrix) override;

private:
    //==============================================================================
    HeadPanel::HeadPanel headPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    headPanel.setListener(this);
    headPanel.setTopLeftPosition(8, 8);
    addAndMakeVisible(headPanel);
    setSize(headPanel.getWidth()+16, headPanel.getHeight()+16);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // Note that the default JUCE look and feel colour is used here. The component works
    // best against dark backgrounds such as this: there's no 'light mode' at the moment.
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::trackerChanged(const HeadMatrix& headMatrix)
{
    // headMatrix.transform and headMatrix.transformTranspose can be used here
    // to rotate an object.
}
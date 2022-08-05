/*
 * Head tracker configuration panels
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace ConfigPanel
{
    class SettingsPanel: public BasePanel
    {
    public:
        SettingsPanel(Midi::TrackerDriver& trackerDriver):
            BasePanel(""),
            td(trackerDriver)
        {
            juce::Point<int> position(4, yOrigin());
            addLabel(position, "Chirality", LabelStyle::SectionHeading);
            addToggle(position, "Cable on left", 1);
            addToggle(position, "Cable on right", 1);

            addLabel(position, "Yaw correction", LabelStyle::SectionHeading);
            addToggle(position, "Use the compass", 2);
            addToggle(position, "Slow central pull", 2);

            position.addXY(0,2);
            juce::Point<int> rightPos = position;
            rightPos.addXY(132, 26);
            addTextButton(position, "Calibrate compass", 172);
            addLabel(rightPos, "", LabelStyle::SubData);
            position.addXY(0, 14);

            addLabel(position, "Travel mode (not preserved)", LabelStyle::SectionHeading);
            addToggle(position, "Off", 3);
            addToggle(position, "Slow correction", 3);
            addToggle(position, "Fast correction", 3);

            setSize(LabelWidth, position.y + 2);
            setEnabled(false);
        }

        // ---------------------------------------------------------------------

        void setEnabled(const bool isConnected)
        {
            enablePanel(isConnected);
        }

        // ---------------------------------------------------------------------

        void click(const bool isTextButton, const int index, const bool isChecked) override
        {
            if (isTextButton)
            {
                // button is disabled via the readback
                td.calibrateCompass();
            }
            else
            {
                switch(index)
                {
                case 0: if (isChecked) td.setChirality(false);  break;
                case 1: if (isChecked) td.setChirality(true);   break;
                case 2: if (isChecked) td.setCompass(true);    break;
                case 3: if (isChecked) td.setCompass(false);   break;
                case 4: if (isChecked) td.setTravelMode(Tracker::TravelMode::Off);     break;
                case 5: if (isChecked) td.setTravelMode(Tracker::TravelMode::Slow);    break;
                case 6: if (isChecked) td.setTravelMode(Tracker::TravelMode::Fast);    break;
                }
            }
        }

        // ---------------------------------------------------------------------

        void timerCallback(int timerID) override
        {
            if (timerID == 1)
            {
                setCompassLabel();
                juce::MultiTimer::stopTimer(timerID);
            }
            BasePanel::timerCallback(timerID);
        }

        // ---------------------------------------------------------------------

        void setCompassState(Tracker::CompassState newCompassState)
        {
            compassState = newCompassState;
            setCompassLabel();
        }

        // ---------------------------------------------------------------------

        void trackConnectionState(Midi::State /*newState*/)
        {
            setEnabled(td.isConnected());
            setCompassLabel();
        }

        // ---------------------------------------------------------------------

        void trackCompassState(Tracker::CompassState newCompassState)
        {
            if (compassState != newCompassState)
            {
                compassState = newCompassState;
                setCompassLabel();
            }
        }

        // ---------------------------------------------------------------------

        void trackUpdatedState(bool rightEarChirality, bool compassOn, Tracker::TravelMode travelMode)
        {
            juce::MessageManagerLock mml;
            bool isConnected = td.isConnected();
            setEnabled(isConnected);
            if(isConnected)
            {
                toggleButtons[0]->setToggleState(!rightEarChirality, juce::dontSendNotification);
                toggleButtons[1]->setToggleState(rightEarChirality, juce::dontSendNotification);
                //
                toggleButtons[2]->setToggleState(compassOn, juce::dontSendNotification);
                toggleButtons[3]->setToggleState(!compassOn, juce::dontSendNotification);
                //
                toggleButtons[4]->setToggleState(travelMode == Tracker::TravelMode::Off, juce::dontSendNotification);
                toggleButtons[5]->setToggleState(travelMode == Tracker::TravelMode::Slow, juce::dontSendNotification);
                toggleButtons[6]->setToggleState(travelMode == Tracker::TravelMode::Fast, juce::dontSendNotification);
            }
            flagRepaint();
        }

        // ---------------------------------------------------------------------

    private:
        Midi::TrackerDriver& td;
        Tracker::CompassState compassState;

        // ---------------------------------------------------------------------

        void setCompassLabel()
        {
            juce::MessageManagerLock mml; // needed for setEnabled
            juce::String labelText = "";
            if (td.isConnected())
            {
                if (compassState == Tracker::CompassState::Calibrating)    labelText = "CALIBRATING";
                else if (compassState == Tracker::CompassState::Succeeded) labelText = "SUCCEEDED";
                else if (compassState == Tracker::CompassState::Failed)    labelText = "FAILED";
                else if (compassState == Tracker::CompassState::GoodData)  labelText = "GOOD DATA";
                else if (compassState == Tracker::CompassState::BadData)   labelText = "BAD DATA";
            }
            labels[2]->setText(labelText, juce::dontSendNotification);
            textButtons[0]->setEnabled(compassState != Tracker::CompassState::Calibrating);
        }
    };
};

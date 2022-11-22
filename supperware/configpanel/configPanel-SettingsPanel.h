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
            BasePanel(trackerDriver, nullptr, ""),
            compassState(Tracker::CompassState::Off)
        {
            juce::Point<int> position(4, yOrigin());
            addLabel(position, "Chirality", LabelStyle::SectionHeading);
            addToggle(position, "Cable on left", 1);
            addToggle(position, "Cable on right", 1);

            addLabel(position, "Compass", LabelStyle::SectionHeading);
            addToggle(position, "Use the compass", -1);
            
            position.addXY(0, 1);
            juce::Point<int> rightPos = position;
            rightPos.addXY(132, 24);
            addTextButton(position, "Calibrate compass", 172);
            addLabel(rightPos, "", LabelStyle::Data);
            
            position.addXY(0, 10);
            addLabel(position, "Correction without compass", LabelStyle::SectionHeading);
            addToggle(position, "Slow central pull", -1);

            addLabel(position, "Travel mode (not preserved)", LabelStyle::SectionHeading);
            addToggle(position, "Off", 3);
            addToggle(position, "Slow correction", 3);
            addToggle(position, "Fast correction", 3);

            setSize(LabelWidth, position.y + 2);
            setEnabled(false);
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
                case 2: if (isChecked) td.setCompass(isChecked, toggleButtons[3]->getToggleState());   break;
                case 3: if (isChecked) td.setCompass(toggleButtons[2]->getToggleState(), isChecked);   break;
                case 4: if (isChecked) td.setTravelMode(Tracker::TravelMode::Off);     break;
                case 5: if (isChecked) td.setTravelMode(Tracker::TravelMode::Slow);    break;
                case 6: if (isChecked) td.setTravelMode(Tracker::TravelMode::Fast);    break;
                }
            }
        }

        // ---------------------------------------------------------------------

        void trackerMidiConnectionChanged(const Midi::State& /*state*/) override
        {
            setEnabled(td.isConnected());
            refreshAsync();
        }

        // ---------------------------------------------------------------------

        void trackerCompassStateChanged(Tracker::CompassState newCompassState) override
        {
            if (compassState != newCompassState)
            {
                compassState = newCompassState;
                refreshAsync();
            }
        }

        // ---------------------------------------------------------------------

        void trackUpdatedState(bool /*rightEarChirality*/, bool /*compassOn*/, Tracker::TravelMode /*travelMode*/)
        {
            refreshAsync();
        }

        // ---------------------------------------------------------------------

    private:
        Tracker::CompassState compassState;

        // ---------------------------------------------------------------------

        void refreshAsync()
        {
            startTimer(1, 17); // 60Hz
        }

        // ---------------------------------------------------------------------

        void timerCallback(int timerID) override
        {
            BasePanel::timerCallback(timerID);

            if (timerID == 1)
            {
                stopTimer(1);

                juce::String labelText;
                bool isConnected = td.isConnected();
                Tracker::State state = td.getState();
                setEnabled(isConnected);
                if (isConnected)
                {
                    toggleButtons[0]->setToggleState(!state.rightEarChirality, juce::dontSendNotification);
                    toggleButtons[1]->setToggleState(state.rightEarChirality, juce::dontSendNotification);
                    //
                    toggleButtons[2]->setToggleState(state.compassOn, juce::dontSendNotification);
                    toggleButtons[3]->setToggleState(state.compassSlowCorrection, juce::dontSendNotification);
                    //
                    toggleButtons[4]->setToggleState(state.travelMode == Tracker::TravelMode::Off, juce::dontSendNotification);
                    toggleButtons[5]->setToggleState(state.travelMode == Tracker::TravelMode::Slow, juce::dontSendNotification);
                    toggleButtons[6]->setToggleState(state.travelMode == Tracker::TravelMode::Fast, juce::dontSendNotification);
                }
                repaintAsync();

                if (td.isConnected())
                {
                    switch (state.compassState)
                    {
                    case Tracker::CompassState::Off:          labelText = "[ OFF ]";      break;
                    case Tracker::CompassState::Calibrating:  labelText = "CALIBRATING";  break;
                    case Tracker::CompassState::Succeeded:    labelText = "SUCCEEDED";    break;
                    case Tracker::CompassState::Failed:       labelText = "FAILED";       break;
                    case Tracker::CompassState::GoodData:     labelText = "GOOD DATA";    break;
                    case Tracker::CompassState::BadData:      labelText = "BAD DATA";     break;
                    default: labelText = "";
                    }
                }
                labels[2]->setText(labelText, juce::dontSendNotification);
                textButtons[0]->setEnabled(state.compassState != Tracker::CompassState::Calibrating);
            }
        }
    };
};

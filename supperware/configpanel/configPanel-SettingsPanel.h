/*
 * Head tracker configuration panels
 * Copyright 2021 Supperware Ltd.
 */

#pragma once

namespace ConfigPanel
{
    class SettingsPanel: public BasePanel
    {
    public:
        SettingsPanel(Midi::Tracker& headTracker):
            BasePanel(""),
            ht(headTracker)
        {
            Point<int> position(4, yOrigin());
            addLabel(position, "Chirality", LabelStyle::SectionHeading);
            addToggle(position, "Cable on left", 1);
            addToggle(position, "Cable on right", 1);

            addLabel(position, "Yaw correction", LabelStyle::SectionHeading);
            addToggle(position, "Use the compass", 2);
            addToggle(position, "Slow central pull", 2);

            position.addXY(0,2);
            Point<int> rightPos = position;
            rightPos.addXY(132, 26);
            addTextButton(position, "Calibrate compass", 172);
            addLabel(rightPos, "", LabelStyle::SubData);
            position.addXY(0, 14);

            addLabel(position, "Travel mode (not preserved;", LabelStyle::SectionHeading);
            position.addXY(0, -14);
            addLabel(position, "inactive when using compass)", LabelStyle::SectionHeading);
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
                ht.calibrateCompass();
            }
            else
            {
                switch(index)
                {
                case 0: if (isChecked) ht.setChirality(false);  break;
                case 1: if (isChecked) ht.setChirality(true);   break;
                case 2: if (isChecked) ht.setCompass(true);    break;
                case 3: if (isChecked) ht.setCompass(false);   break;
                case 4: if (isChecked) ht.setTravelMode(Midi::TravelMode::Off);     break;
                case 5: if (isChecked) ht.setTravelMode(Midi::TravelMode::Slow);    break;
                case 6: if (isChecked) ht.setTravelMode(Midi::TravelMode::Fast);    break;
                }
            }
        }

        // ---------------------------------------------------------------------

        void timerCallback(int timerID) override
        {
            if (timerID == 1)
            {
                setCompassLabel();
                MultiTimer::stopTimer(timerID);
            }
            BasePanel::timerCallback(timerID);
        }

        // ---------------------------------------------------------------------

        void setCompassState(Midi::CompassState newCompassState)
        {
            compassState = newCompassState;
            setCompassLabel();
        }

        // ---------------------------------------------------------------------

        void trackConnectionState(Midi::State /*newState*/)
        {
            setEnabled(ht.isConnected());
            setCompassLabel();
        }

        // ---------------------------------------------------------------------

        void trackCompassState(Midi::CompassState newCompassState)
        {
            if (compassState != newCompassState)
            {
                compassState = newCompassState;
                setCompassLabel();
            }
        }

        // ---------------------------------------------------------------------

        void trackUpdatedState(bool rightEarChirality, bool compassOn, Midi::TravelMode travelMode)
        {
            MessageManagerLock mml;
            bool isConnected = ht.isConnected();
            setEnabled(isConnected);
            if(isConnected)
            {
                toggleButtons[0]->setToggleState(!rightEarChirality, dontSendNotification);
                toggleButtons[1]->setToggleState(rightEarChirality, dontSendNotification);
                //
                toggleButtons[2]->setToggleState(compassOn, dontSendNotification);
                toggleButtons[3]->setToggleState(!compassOn, dontSendNotification);
                //
                toggleButtons[4]->setToggleState(travelMode == Midi::TravelMode::Off, dontSendNotification);
                toggleButtons[5]->setToggleState(travelMode == Midi::TravelMode::Slow, dontSendNotification);
                toggleButtons[6]->setToggleState(travelMode == Midi::TravelMode::Fast, dontSendNotification);
            }
            flagRepaint();
        }

        // ---------------------------------------------------------------------

    private:
        Midi::Tracker& ht;
        Midi::CompassState compassState;

        // ---------------------------------------------------------------------

        void setCompassLabel()
        {
            MessageManagerLock mml; // needed for setEnabled
            String labelText = "";
            if (ht.isConnected())
            {
                if (compassState == Midi::CompassState::Calibrating)    labelText = "CALIBRATING";
                else if (compassState == Midi::CompassState::Succeeded) labelText = "SUCCEEDED";
                else if (compassState == Midi::CompassState::Failed)    labelText = "FAILED";
                else if (compassState == Midi::CompassState::GoodData)  labelText = "GOOD DATA";
                else if (compassState == Midi::CompassState::BadData)   labelText = "BAD DATA";
            }
            labels[2]->setText(labelText, dontSendNotification);
            textButtons[0]->setEnabled(compassState != Midi::CompassState::Calibrating);
        }
    };
};

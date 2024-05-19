/*
 * MIDI drivers
 * Supperware head tracker driver
 * see https://supperware.co.uk/headtracker for MIDI specification
 * Copyright (c) 2021-3 Supperware Ltd.
 */

#pragma once

namespace Midi
{
    class TrackerDriver: public MidiDuplex, Tracker::Listener
    {
    public:
        class Listener
        {
        public:
            virtual ~Listener() {};
            
            /** Callbacks inherited from Tracker. */
            virtual void trackerOrientation(float /*yawRadian*/, float /*pitchRadian*/, float /*rollRadian*/) {}
            virtual void trackerOrientationQ(float /*qw*/, float /*qx*/, float /*qy*/, float /*qz*/) {}
            virtual void trackerOrientationM(float* /*matrix*/) {}
            virtual void trackerCompassStateChanged(Tracker::CompassState /*compassState*/) {}
            virtual void trackerConnectionChanged(const Tracker::State& /*state*/) {}

            /** Called when the head tracker's connection state or its status data is changed */
            virtual void trackerMidiConnectionChanged(Midi::State /*state*/) {}
        };

        TrackerDriver() :
            MidiDuplex("Head Tracker", "Supperware Bootloader"),
            tracker(this),
            currentAngleMode(Tracker::AngleMode::Quaternion),
            is100Hz(false),
            isTrackerOn(false)
        {}

        // ------------------------------------------------------------------------

        // pass through to our listener
        void trackerOrientation(float yawRadian, float pitchRadian, float rollRadian) override
        {
            for (Listener* l: listeners)
            {
                l->trackerOrientation(yawRadian, pitchRadian, rollRadian);
            }
        }
        void trackerOrientationQ(float qw, float qx, float qy, float qz) override
        {
            for (Listener* l: listeners)
            {
                l->trackerOrientationQ(qw, qx, qy, qz);
            }
        }
        void trackerOrientationM(float* matrix) override
        {
            for (Listener* l: listeners)
            {
                l->trackerOrientationM(matrix);
            }
        }
        void trackerCompassStateChanged(Tracker::CompassState compassState) override
        {
            for (Listener* l: listeners)
            {
                l->trackerCompassStateChanged(compassState);
            }
        }
        void trackerConnectionChanged(const Tracker::State& state) override
        {
            for (Listener* l: listeners)
            {
                l->trackerConnectionChanged(state);
            }
        }

        // ------------------------------------------------------------------------

        void addListener(Listener* listener)
        {
            listeners.push_back(listener);
        }

        // ------------------------------------------------------------------------

        const Tracker::State& getState() const
        {
            return tracker.getState();
        }

        // ------------------------------------------------------------------------

        /** Stops sending data, without disconnecting. */
        void turnOff()
        {
            if (isTrackerOn)
            {
                isTrackerOn = false;
                size_t numBytes = tracker.turnOffMessage(midiBuffer);
                sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
            }
        }

        // ------------------------------------------------------------------------

        /** If set100Hz is false, the tracker responds at 50Hz.
            These settings are remembered if you enable setAutoDisconnect / setAutoReconnect. */
        void turnOn(bool is100HzMode = false, bool isQuaternionMode = true)
        {
            if (connectionState != State::Connected)
            {
                connect();
            }

            currentAngleMode = isQuaternionMode ? Tracker::AngleMode::Quaternion : Tracker::AngleMode::YPR;

            if (connectionState == State::Connected)
            {
                is100Hz = is100HzMode;
                isTrackerOn = true;
                size_t numBytes = tracker.turnOnMessage(midiBuffer, currentAngleMode, is100Hz);
                sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
            }
        }

        // ------------------------------------------------------------------------

        /** Centres the head tracker. */
        void zero()
        {
            size_t numBytes = tracker.zeroMessage(midiBuffer);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }

        // ------------------------------------------------------------------------

        /** Determines whether the cable should be over the left or right ear. */
        void setChirality(const bool isRightEarChirality)
        {
            size_t numBytes = tracker.chiralityMessage(midiBuffer, isRightEarChirality);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }
        
        // ------------------------------------------------------------------------

        /** Automatic zeroing modes (work only when the compass is off). */
        void setTravelMode(const Tracker::TravelMode newTravelMode)
        {
            size_t numBytes = tracker.travelModeMessage(midiBuffer, newTravelMode);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }

        // ------------------------------------------------------------------------

        /** Turn compass on or off.
            This state can be read back with getCompassState(). */
        void setCompass(bool compassShouldBeOn, bool compassShouldApplyYawCorrection)
        {
            size_t numBytes = tracker.compassMessage(midiBuffer, compassShouldBeOn, compassShouldApplyYawCorrection);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }
        
        // ------------------------------------------------------------------------

        /** Turn shake-to-zero gesture on or off. */
        void setGestures(bool shakeToZero)
        {
            size_t numBytes = tracker.gestureMessage(midiBuffer, shakeToZero);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }

        // ------------------------------------------------------------------------

        /** Change central pull speed. */
        void setPullSpeed(unsigned char pullSpeed)
        {
            size_t numBytes = tracker.pullSpeedMessage(midiBuffer, pullSpeed);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }

        // ------------------------------------------------------------------------
        /** Put compass in calibration mode. */
        void calibrateCompass()
        {
            size_t numBytes = tracker.calibrateCompassMessage(midiBuffer);
            sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
        }

        // ------------------------------------------------------------------------

    protected:
        virtual void handleOtherSysEx(const uint8_t* /*buffer*/, const size_t /*numBytes*/) {}

        // ------------------------------------------------------------------------

        void handleSysEx(const uint8_t* data, const size_t numBytes) override
        {
            if (!tracker.processSysex(data, numBytes))
            {
                handleOtherSysEx(data, numBytes);
            }
        }

        // ------------------------------------------------------------------------

        void connectionStateChanged() override
        {
            if (connectionState == State::Connected)
            {
                size_t numBytes = tracker.readbackMessage(midiBuffer);
                sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
            }
            for (Listener* l: listeners)
            {
                l->trackerMidiConnectionChanged(connectionState);
            }
        }

        // ------------------------------------------------------------------------

    private:
        std::vector<Listener*> listeners;
        Tracker tracker;
        juce::Vector3D<float> position;
        uint8_t midiBuffer[16];
        Tracker::AngleMode currentAngleMode;
        bool is100Hz;
        bool isTrackerOn;
    };
};

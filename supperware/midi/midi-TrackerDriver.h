/*
 * MIDI drivers
 * Supperware head tracker driver
 * see https://supperware.co.uk/headtracker for MIDI specification
 * Copyright (c) 2021 Supperware Ltd.
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
            virtual void trackerCompassStateChanged(Tracker::CompassState /*compassState*/) {}
            virtual void trackerConnectionChanged(const Tracker::State& /*state*/) {}
            
            /** Called when the head tracker's connection state or its status data is changed */
            virtual void trackerMidiConnectionChanged(const Midi::State /*state*/) {}
        };

        /** You must nominate a Tracker::Listener in the constructor. This is usually the parent class. */
        TrackerDriver(Listener* listener) :
            MidiDuplex("Head Tracker", "Supperware Bootloader"),
            l(listener),
            tracker(this),
            isQuaternion(true),
            is100Hz(false),
            isTrackerOn(false)
        {}

        // ------------------------------------------------------------------------

        // pass through to our listener
        void trackerOrientation(float yawRadian, float pitchRadian, float rollRadian) override
        {
            l->trackerOrientation(yawRadian, pitchRadian, rollRadian);
        }
        void trackerOrientationQ(float qw, float qx, float qy, float qz) override
        {
            l->trackerOrientationQ(qw, qx, qy, qz);
        }
        void trackerCompassStateChanged(Tracker::CompassState compassState) override
        {
            l->trackerCompassStateChanged(compassState);
        }
        void trackerConnectionChanged(const Tracker::State& state) override
        {
            l->trackerConnectionChanged(state);
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
            if (connectionState == State::Connected)
            {
                is100Hz = is100HzMode;
                isQuaternion = isQuaternionMode;
                isTrackerOn = true;
                size_t numBytes = tracker.turnOnMessage(midiBuffer, isQuaternion, is100Hz);
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

        /** Turn compass on or off (when it's off, it's in 'slow central pull' mode).
            This state can be read back with getCompassState(). */
        void setCompass(bool compassShouldBeOn)
        {
            size_t numBytes = tracker.compassMessage(midiBuffer, compassShouldBeOn);
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

        void handleSysEx(const uint8_t* buffer, const size_t numBytes) override
        {
            if (!tracker.processSysex(buffer, numBytes))
            {
                handleOtherSysEx(buffer, numBytes);
            }
        }

        // ------------------------------------------------------------------------

        void connectionStateChanged() override
        {
            if (connectionState == State::Connected)
            {
                if (isTrackerOn)
                {
                    size_t numBytes = tracker.turnOnMessage(midiBuffer, isQuaternion, is100Hz);
                    sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
                }
                size_t numBytes = tracker.readbackMessage(midiBuffer);
                sendMessage(juce::MidiMessage(midiBuffer, (int)numBytes));
            }
            l->trackerMidiConnectionChanged(connectionState);
        }

        // ------------------------------------------------------------------------

    private:
        Listener* l;
        Tracker tracker;
        juce::Vector3D<float> position;
        uint8_t midiBuffer[16];
        bool isQuaternion;
        bool is100Hz;
        bool isTrackerOn;
    };
};

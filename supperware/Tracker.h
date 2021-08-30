/*
 * MIDI drivers
 * Head tracker state and MIDI processing.
 * This class doesn't need JUCE!
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

class Tracker
{
public:
    enum UpdateMode { DontUpdateState, UpdateWithoutNotifying, NotifyListener };
    enum class CompassState { Off, Calibrating, Succeeded, Failed, GoodData, BadData };
    enum class TravelMode { Off, Slow, Fast };

    // ------------------------------------------------------------------------

    struct State
    {
        bool rightEarChirality;
        bool compassOn;
        bool gestureShakeToCalibrate;
        bool gestureTapToZero; 
        TravelMode travelMode;
        CompassState compassState;

        State() :
            rightEarChirality(false),
            compassOn(false),
            gestureShakeToCalibrate(false),
            gestureTapToZero(false),
            travelMode(TravelMode::Off),
            compassState(CompassState::Off)
        {}
    };

    // ------------------------------------------------------------------------

    class Listener
    {
    public:
        virtual ~Listener() {};

        // The head tracker sends only one of these, depending what you ask for in turnOn().
        /** Yaw/Pitch/Roll. */
        virtual void trackerOrientation(float /*yawRadian*/, float /*pitchRadian*/, float /*rollRadian*/) {}
        /** Quaternions. */
        virtual void trackerOrientationQ(float /*qw*/, float /*qx*/, float /*qy*/, float /*qz*/) {}

        /** Called when the compass state changes */
        virtual void trackerCompassStateChanged(CompassState /*compassState*/) {}

        /** Called when the head tracker's connection state or its status data is changed */
        virtual void trackerConnectionChanged(const State& /*state*/) {}
    };

    // ------------------------------------------------------------------------

    Tracker() :
        l(nullptr)
    {}

    // ------------------------------------------------------------------------

    Tracker(Listener* listener) : 
        l(listener)
    {}

    // ------------------------------------------------------------------------

    /** there can be only one listener */
    void setListener(Listener* listener)
    {
        l = listener;
    }

    // ------------------------------------------------------------------------

    /** formats a System Exclusive message to turn on the head tracker,
        and returns the size in bytes. If quaternionMode is false, the
        yaw/pitch/roll callback will be used. If use100Hz is false, the
        head tracker will respond at 50Hz. */
    size_t turnOnMessage(uint8_t* buffer, bool quaternionMode, bool use100Hz) const
    {
        constexpr int MessageLength = 12;
        supperwareSysex(buffer, MessageLength);
        buffer[4] = 0x00; // Message 0 : Configure sensors and processing pipeline
        buffer[5] = 0x00; // - Parameter 0 : Sensor setup
        buffer[6] = (use100Hz) ? 0x28 : 0x08;
        buffer[7] = 0x01; // - Parameter 1 : Data output and formatting
        buffer[8] = (quaternionMode) ? 0x05 : 0x01;
        buffer[9] = 0x03; // - Parameter 3 : Magnetometer control (to set verbose)
        buffer[10] = 0x40;
        return MessageLength;
    }

    // ------------------------------------------------------------------------

    /** formats a System Exclusive message to turn off the head tracker,
        and returns the size in bytes */
    size_t turnOffMessage(uint8_t* buffer) const
    {
        return singleValueSysex(buffer, 0x00, 0x00, 0x40);
    }

    // ------------------------------------------------------------------------

    /** formats a System Exclusive message to zero the head tracker,
        and returns the size in bytes */
    size_t zeroMessage(uint8_t* buffer) const
    {
        return singleValueSysex(buffer, 0x01, 0x00, 0x01);
    }

    // ------------------------------------------------------------------------

    /** formats a System Exclusive message to determine whether the cable should be
        over the left or right ear. */
    size_t chiralityMessage(uint8_t* buffer, bool isRightEarChirality, UpdateMode updateMode = UpdateWithoutNotifying)
    {
        if ((updateMode != DontUpdateState) && (state.rightEarChirality != isRightEarChirality))
        {
            state.rightEarChirality = isRightEarChirality;
            notifyIfNecessary(updateMode);
        }
        return singleValueSysex(buffer, 0x00, 0x04, (isRightEarChirality) ? 0x03 : 0x02);
    }

    // ------------------------------------------------------------------------

    /** formats a System Exclusive message to set the automatic zeroing [travel] mode
        (works only when the compass is off). */
    size_t travelModeMessage(uint8_t* buffer, const TravelMode newTravelMode, UpdateMode updateMode = UpdateWithoutNotifying)
    {
        if ((updateMode != DontUpdateState) && (state.travelMode != newTravelMode))
        {
            state.travelMode = newTravelMode;
            notifyIfNecessary(updateMode);
        }
        uint8_t travelByte;
        /**/ if (newTravelMode == TravelMode::Slow) { travelByte = 0x06; }
        else if (newTravelMode == TravelMode::Fast) { travelByte = 0x07; }
        else { travelByte = 0x04; }
        return singleValueSysex(buffer, 0x01, 0x01, travelByte);
    }

    // ------------------------------------------------------------------------

    /** ormats a System Exclusive message to turn the cmopass on or off
        (when it's off, it's in 'slow central pull' mode). */
    size_t compassMessage(uint8_t* buffer, bool compassShouldBeOn, UpdateMode updateMode = UpdateWithoutNotifying)
    {
        if ((updateMode != DontUpdateState) && (state.compassOn != compassShouldBeOn))
        {
            state.compassOn = compassShouldBeOn;
            notifyIfNecessary(updateMode);
        }
        return singleValueSysex(buffer, 0x00, 0x03, (compassShouldBeOn) ? 0x70 : 0x60);
    }

    // ------------------------------------------------------------------------

    /** Put compass in calibration mode. */
    size_t calibrateCompassMessage(uint8_t* buffer) const
    {
        return singleValueSysex(buffer, 0x00, 0x03, 0x44);
    }

    // ------------------------------------------------------------------------

    /** This should be sent to get the head tracker status whenever a device is newly connected */
    size_t readbackMessage(uint8_t* buffer) const
    {
        constexpr int MessageLength = 9;
        supperwareSysex(buffer, MessageLength);
        buffer[4] = 0x02; // Message 2 : Readback
        buffer[5] = 0x03; // -- Magnetometer
        buffer[6] = 0x04; // -- Gesture and chirality
        buffer[7] = 0x11; // -- Travel mode
        return MessageLength;
    }

    // ------------------------------------------------------------------------

    /** The buffer passed to this call and the byte count should be stripped of
        the leading 0xF0 and trailing 0xF7. Returns true if we handle the 
        message. */
    bool processSysex(const uint8_t* buffer, size_t numBytes)
    {
        if (sysexMatch(buffer, numBytes, 11, 0x40, 0x00))
        {
            float yawRadian = bytes211ToFloat(buffer + 5);
            float pitchRadian = bytes211ToFloat(buffer + 7);
            float rollRadian = bytes211ToFloat(buffer + 9);
            if (l) l->trackerOrientation(yawRadian, pitchRadian, rollRadian);
            return true;
        }
        if (sysexMatch(buffer, numBytes, 13, 0x40, 0x01))
        {
            float qw = bytes211ToFloat(buffer + 5);
            float qx = bytes211ToFloat(buffer + 7);
            float qy = bytes211ToFloat(buffer + 9);
            float qz = bytes211ToFloat(buffer + 11);
            if (l) l->trackerOrientationQ(qw, qx, qy, qz);
            return true;
        }
        else if (buffer[3] == 0x42 && (numBytes >= 6) && !(numBytes & 1))
        {
            // readback; even number of bytes; at least 6.
            for (uint8_t i = 4; i < numBytes; i += 2)
            {
                processReadback(buffer[i], buffer[i+1]);
            }
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------------------

    /** Retrieve the current state of the head tracker: usually as a response*/
    const State& getState() const
    {
        return state;
    }

    // ------------------------------------------------------------------------

private:
    State state;
    Listener* l;

    // ------------------------------------------------------------------------

    void supperwareSysex(uint8_t* buffer, uint8_t size) const
    {
        const uint8_t preamble[4] = { 0xf0, 0x00, 0x21, 0x42 };
        memcpy(buffer, preamble, sizeof(preamble));
        buffer[size-1] = 0xf7;
    }

    // ------------------------------------------------------------------------

    size_t singleValueSysex(uint8_t* buffer, uint8_t message, uint8_t parameter, uint8_t value) const
    {
        constexpr int MessageLength = 8;
        supperwareSysex(buffer, MessageLength);
        buffer[4] = message;
        buffer[5] = parameter;
        buffer[6] = value;
        return MessageLength;
    }

    // ------------------------------------------------------------------------

    /** converts Q2.11 format to a floating-point number */
    static float bytes211ToFloat(const uint8_t* buffer) noexcept
    {
        int w = (buffer[0] << 7) + buffer[1];
        if (w >= 0x2000) w -= 0x4000;
        float f = static_cast<float>(w) / 2048.0f;
        return f;
    }

    // ------------------------------------------------------------------------

    void notifyIfNecessary(UpdateMode updateMode)
    {
        if ((updateMode == NotifyListener) && l)
        {
            l->trackerConnectionChanged(state);
        }
    }

    // ------------------------------------------------------------------------

    static bool sysexMatch(const uint8_t* buffer, size_t numBytes,
        size_t numBytesToMatch, uint8_t messageNumber) noexcept
    {
        if (numBytes != numBytesToMatch) return false;
        if (buffer[3] != messageNumber) return false;
        return true;
    }

    // ------------------------------------------------------------------------

    static bool sysexMatch(const uint8_t* buffer, size_t numBytes,
        size_t numBytesToMatch, uint8_t messageNumber,
        uint8_t parameterNumber) noexcept
    {
        if (!sysexMatch(buffer, numBytes, numBytesToMatch, messageNumber))
        {
            return false;
        }
        return (buffer[4] == parameterNumber);
    }

    // ------------------------------------------------------------------------

    void processReadback(const uint8_t parameter, const uint8_t value)
    {
        if (parameter == 0x03)
        {
            // compass control
            state.compassOn = (value & 0x38) >= 0x30;
            switch (value & 3)
            {
            case 0: state.compassState = CompassState::Off; break;
            case 1: state.compassState = CompassState::BadData; break;
            case 2: state.compassState = CompassState::GoodData; break;
            default: state.compassState = CompassState::Calibrating; break;
            }
            if (l) l->trackerCompassStateChanged(state.compassState);
        }
        else if (parameter == 0x04)
        {
            state.rightEarChirality = (value & 3) == 3;
            state.gestureShakeToCalibrate = (value & 0x14) == 0x14;
            state.gestureTapToZero = (value & 0x18) == 0x18;
        }
        else if (parameter == 0x05)
        {
            switch (value)
            {
            case 1: state.compassState = CompassState::Calibrating; break;
            case 2: state.compassState = CompassState::Succeeded; break;
            case 3: state.compassState = CompassState::Failed; break;
            case 4: state.compassState = CompassState::BadData; break;
            case 5: state.compassState = CompassState::GoodData; break;
            }
            if (l) l->trackerCompassStateChanged(state.compassState);
        }
        else if (parameter == 0x11)
        {
            /**/ if ((value & 7) == 7) state.travelMode = TravelMode::Fast;
            else if ((value & 7) == 6) state.travelMode = TravelMode::Slow;
            else state.travelMode = TravelMode::Off;
            // this message will be received last if they're all requested,
            // so send an update now.
            if (l) l->trackerConnectionChanged(state);
        }
    }
};

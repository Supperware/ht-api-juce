/*
 * MIDI drivers
 * Supperware head tracker driver
 * see https://supperware.co.uk/headtracker for MIDI specification
 * Copyright 2021 Supperware Ltd.
 */

#pragma once

namespace Midi
{
    enum class CompassState { Off, Calibrating, Succeeded, Failed, GoodData, BadData };
    enum class TravelMode { Off, Slow, Fast };
    enum class AngleFormat { YPR, Quaternion, Matrix };

    class Tracker: public MidiDuplex
    {
    public:
        class Receiver
        {
        public:
            virtual ~Receiver() {};
            
            // The head tracker sends only one of these, depending what you ask for in turnOn().
            /** Yaw/Pitch/Roll. */
            virtual void trackOrientation(float /*yawRadian*/, float /*pitchRadian*/, float /*rollRadian*/) {}
            /** Quaternions. */
            virtual void trackOrientationQ(float /*qw*/, float /*qx*/, float /*qy*/, float /*qz*/) {}
            /** Rotation matrix. */
            virtual void trackOrientationM(Vector3D<float> /*x*/, Vector3D<float> /*y*/, Vector3D<float> /*z*/) {}

            /** Called when the MIDI device connects or disconnects */
            virtual void trackConnectionState(Midi::State /*newState*/) {}

            /** Called when the compass state changes */
            virtual void trackCompassState(Midi::CompassState /*compassState*/) {}
            
            /** Called when the head tracker is newly plugged in and all the status data is refreshed */
            virtual void trackUpdatedState(bool /*rightEarChirality*/, bool /*compassOn*/, Midi::TravelMode /*travelMode*/) {}
        };

        /** You must nominate a Tracker::Receiver in the constructor. This is usually the parent class. */
        Tracker(Receiver* dataReceiver) :
            MidiDuplex("Head Tracker", "Supperware Bootloader"),
            receiver(dataReceiver),
            set100Hz(false), trackerOn(false), rightEarChirality(false), compassOn(false),
            gestureShake(false), gestureTap(false),
            angleFormat(AngleFormat::YPR),
            compassState(CompassState::Off),
            travelMode(TravelMode::Off)
        {}

        // ------------------------------------------------------------------------

        /** Stops sending data, without disconnecting. */
        void turnOff()
        {
            if (trackerOn)
            {
                trackerOn = false;
                const uint8 M[] = { 0xf0, 0x00, 0x21, 0x42, 0x00, 0x00, 0x40, 0xf7 };
                sendMessage(MidiMessage(M, sizeof(M)));
            }
        }

        // ------------------------------------------------------------------------

        /** If set100Hz is false, the tracker responds at 50Hz.
          * These settings are remembered if you enable setAutoDisconnect / setAutoReconnect. */
        void turnOn(const bool is100Hz = false, const AngleFormat format = AngleFormat::Quaternion)
        {
            set100Hz = is100Hz;
            angleFormat = format;
            trackerOn = true;
            if (connectionState != State::Connected)
            {
                connect();
            }
            turnOnFinish();
        }

        // ------------------------------------------------------------------------

        /** Centres the head tracker. */
        void zero()
        {
            const uint8 m[] = { 0xf0, 0x00, 0x21, 0x42, 0x01, 0x00, 0x01, 0xf7 };
            sendMessage(MidiMessage(m, sizeof(m)));
        }

        // ------------------------------------------------------------------------

        /** Determines whether the cable should be over the left or right ear. */
        void setChirality(const bool isRightEarChirality)
        {
            if (isRightEarChirality != rightEarChirality)
            {
                rightEarChirality = isRightEarChirality;
                sendValue(0x00, 0x04, (rightEarChirality ? 0x03 : 0x02));
            }
        }
        
        // ------------------------------------------------------------------------

        /** Automatic zeroing modes (work only when the compass is off). */
        void setTravelMode(const TravelMode newTravelMode)
        {
            if (newTravelMode != travelMode)
            {
                travelMode = newTravelMode;
                uint8 travelByte;
                /**/ if (travelMode == TravelMode::Slow) { travelByte = 0x06; }
                else if (travelMode == TravelMode::Fast) { travelByte = 0x07; }
                else { travelByte = 0x04; }
                sendValue(0x01, 0x01, travelByte);
            }
        }

        // ------------------------------------------------------------------------

        /** Turn compass on or off (when it's off, it's in 'slow central pull' mode).
            This state can be read back with getCompassState(). */
        void setCompass(bool shouldBeOn)
        {
            if (compassOn != shouldBeOn)
            {
                compassOn = shouldBeOn;
                sendValue(0x00, 0x03, (shouldBeOn ? 0x70 : 0x60));
            }
        }
        
        // ------------------------------------------------------------------------

        /** Put compass in calibration mode. */
        void calibrateCompass()
        {
            sendValue(0x00, 0x03, 0x44);
        }

        // ------------------------------------------------------------------------

        /** The head tracker supports these gestures, but they're experimental.
            These values should be requested suplementally after a trackUpdatedState()
            callback. */
        void getGestures(bool& gestureShakeToCalibrate, bool& gestureTapToZero) const
        {
            gestureShakeToCalibrate = gestureShake;
            gestureTapToZero = gestureTap;
        }

    protected:
        virtual void handleOtherSysEx(const uint8* /*data*/, const size_t /*numBytes*/) {}

        // ------------------------------------------------------------------------

        void handleSysEx(const uint8* data, const size_t numBytes) override
        {
            
            if (sysexMatch(data, numBytes, 0x40, 0x00, 11))
            {
                float yawRadian = bytes211ToFloat(&data[5]);
                float pitchRadian = bytes211ToFloat(&data[7]);
                float rollRadian = bytes211ToFloat(&data[9]);
                receiver->trackOrientation(yawRadian, pitchRadian, rollRadian);
            }
            else if (sysexMatch(data, numBytes, 0x40, 0x01, 13))
            {
                float qw = bytes211ToFloat(&data[5]);
                float qx = bytes211ToFloat(&data[7]);
                float qy = bytes211ToFloat(&data[9]);
                float qz = bytes211ToFloat(&data[11]);
                receiver->trackOrientationQ(qw, qx, qy, qz);
            }
            else if (sysexMatch(data, numBytes, 0x40, 0x02, 23))
            {
                Vector3D<float> x, y, z;
                x.x = bytes211ToFloat(&data[5]);
                x.y = bytes211ToFloat(&data[7]);
                x.z = bytes211ToFloat(&data[9]);
                y.x = bytes211ToFloat(&data[11]);
                y.y = bytes211ToFloat(&data[13]);
                y.z = bytes211ToFloat(&data[15]);
                z.x = bytes211ToFloat(&data[17]);
                z.y = bytes211ToFloat(&data[19]);
                z.z = bytes211ToFloat(&data[21]);
                receiver->trackOrientationM(x, y, z);
            }
            else if (data[3] == 0x42 && (numBytes >= 6) && !(numBytes & 1))
            {
                // readback; even number of bytes; at least 6.
                for (uint8 i = 4; i < numBytes; i += 2)
                {
                    processReadback(data[i], data[i+1]);
                }
            }
            else
            {
                handleOtherSysEx(data, numBytes);
            }
        }

        // ------------------------------------------------------------------------

        static bool sysexMatch(const uint8* data, size_t numBytes, uint8 messageNumber,
            uint8 parameterNumber, uint8 expectedCount) noexcept
        {
            if (numBytes != expectedCount) return false;
            if (data[3] != messageNumber) return false;
            if (data[4] == parameterNumber) return true;
            if (parameterNumber == DontCareAboutParameter) return true;
            return false;
        }

        // ------------------------------------------------------------------------

        void sendValue(uint8 message, uint8 parameter, uint8 value)
        {
            if (isConnected())
            {
                uint8 m[] = { 0xf0, 0x00, 0x21, 0x42, message, parameter, value, 0xf7 };
                sendMessage(MidiMessage(m, sizeof(m)));
            }
        }

        // ------------------------------------------------------------------------

        void sendReadbackRequests()
        {
            if (isConnected())
            {
                uint8 m[] = { 0xf0, 0x00, 0x21, 0x42, 0x02, 0x03, 0x04, 0x11, 0xf7 };
                sendMessage(MidiMessage(m, sizeof(m)));
            }
        }

        // ------------------------------------------------------------------------

        void connectionStateChanged() override
        {
            if (connectionState == State::Connected)
            {
                if (trackerOn)
                {
                    turnOnFinish();
                }

                sendReadbackRequests();
            }
            receiver->trackConnectionState(connectionState);
        }

        // ------------------------------------------------------------------------

    private:
        Receiver* receiver;
        Vector3D<float> position;
        bool set100Hz, trackerOn, rightEarChirality, compassOn;
        bool gestureShake, gestureTap;
        AngleFormat angleFormat;
        CompassState compassState;
        TravelMode travelMode;
        static constexpr uint8 DontCareAboutParameter { 0x80 };

        // ------------------------------------------------------------------------

        static float bytes211ToFloat(const uint8* data) noexcept
        {
            // Q2.11 radians format to degrees
            int w = (data[0] << 7) + data[1];
            if (w >= 0x2000) w -= 0x4000;
            float f = static_cast<float>(w) / 2048.0f;
            return f;
        }

        // ------------------------------------------------------------------------

        void turnOnFinish()
        {
            uint8 m[12] = { 0xf0, 0x00, 0x21, 0x42, 0x00, 0x00, 0x08, 0x01, 0x01, 0x03, 0x40, 0xf7 };
            if (set100Hz) { m[6] |= 0x20; } // otherwise 50Hz
            /**/ if (angleFormat == AngleFormat::Quaternion) { m[8] |= 0x04; }
            else if (angleFormat == AngleFormat::Matrix)     { m[8] |= 0x08; } // otherwise YPR
            sendMessage(MidiMessage(m, 12));
        }

        // ------------------------------------------------------------------------

        void processReadback(const uint8 parameter, const uint8 value)
        {
            if (parameter == 0x03)
            {
                // compass control
                compassOn = (value & 0x38) >= 0x30;
                switch (value & 3)
                {
                case 0: compassState = CompassState::Off; break;
                case 1: compassState = CompassState::BadData; break;
                case 2: compassState = CompassState::GoodData; break;
                default: compassState = CompassState::Calibrating; break;
                }
                receiver->trackCompassState(compassState);
            }
            else if (parameter == 0x04)
            {
                rightEarChirality = (value & 3) == 3;
                gestureShake = (value & 0x14) == 0x14;
                gestureTap = (value & 0x18) == 0x18;
            }
            else if (parameter == 0x05)
            {
                if (value == 1) compassState = CompassState::Calibrating;
                else if (value == 2) compassState = CompassState::Succeeded;
                else if (value == 3) compassState = CompassState::Failed;
                else if (value == 4) compassState = CompassState::BadData;
                else if (value == 5) compassState = CompassState::GoodData;
                receiver->trackCompassState(compassState);
            }
            else if (parameter == 0x11)
            {
                // Travel mode
                if ((value & 7) == 7) travelMode = TravelMode::Fast;
                else if ((value & 7) == 6) travelMode = TravelMode::Slow;
                else travelMode = TravelMode::Off;
                receiver->trackUpdatedState(rightEarChirality, compassOn, travelMode);
            }
        }
    };
};

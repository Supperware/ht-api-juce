/*
 * MIDI drivers
 * Manages input and output objects,
 * to allow simple MIDI control of a device in both directions.
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace Midi
{
    enum class State { Unavailable, Available, Bootloader, Connected };
    enum class Connection { AsBootloader, AsDevice, AsEither };

    class MidiDuplex : public juce::MidiInputCallback, protected juce::MultiTimer
    {
    public:
        MidiDuplex(const juce::String deviceName, const juce::String bootloaderName) :
            midiOut(nullptr),
            midiIn(nullptr),
            device(deviceName),
            bootloader(bootloaderName),
            connectionState(State::Unavailable),
            autoReconnect(false),
            autoDisconnect(true)
        {
            /* This timer doesn't ever stop: it handles unavailable/available signalling
               even when automatic modes are switched off.
               MultiTimer is used here because it's often necessary to use other timers
               in inherited classes. */
            startTimer(0, TimeoutMilliseconds);
         }

        // ------------------------------------------------------------------------

        ~MidiDuplex()
        {
            disconnect();
        }

        // ------------------------------------------------------------------------

        bool canConnect(const Connection option = Connection::AsEither) const
        {
            juce::String outputIdentifier, inputIdentifier;
            bool wouldConnectToBootloader;
            getIdentifiers(wouldConnectToBootloader, outputIdentifier, inputIdentifier);
            if ((option == Connection::AsDevice) && wouldConnectToBootloader)
            {
                return false;
            }
            if ((option == Connection::AsBootloader) && !wouldConnectToBootloader)
            {
                return false;
            }
            return outputIdentifier.isNotEmpty() && inputIdentifier.isNotEmpty();
        }
        
        // ------------------------------------------------------------------------

        State getConnectionState() const
        {
            return connectionState;
        }

        // ------------------------------------------------------------------------

        /** Simplified version of getConnectionState(). */
        bool isConnected() const
        {
            return (connectionState == State::Connected) ||
                   (connectionState == State::Bootloader);
        }

        // ------------------------------------------------------------------------

        /** If this is set to true, the device will be opened as soon as it appears
            on the list of MIDI input and output devices. */
        void setAutoReconnect(const bool automaticReconnect)
        {
            // reconnects when the connection drops
            autoReconnect = automaticReconnect;
            startTimer(0, TimeoutMilliseconds);
        }
        
        // ------------------------------------------------------------------------

        /** If this is set to true, the device will be marked as disconnected if
            traffic stops for the length of TimeoutMilliseconds. For this to work,
            the device would need either active sensing, or a guaranteed frequency
            of traffic. */
        void setAutoDisconnect(const bool automaticDisconnect)
        {
            // disconnects when inbound traffic stops
            autoDisconnect = automaticDisconnect;
            startTimer(0, TimeoutMilliseconds);
        }

        // ------------------------------------------------------------------------

        bool connect()
        {
            juce::String outputIdentifier, inputIdentifier;
            bool connectingToBootloader = false;
            getIdentifiers(connectingToBootloader, outputIdentifier, inputIdentifier);
            disconnect();
            
            if (outputIdentifier.isNotEmpty() && inputIdentifier.isNotEmpty())
            {
                midiOut = juce::MidiOutput::openDevice(outputIdentifier);
                midiIn  = juce::MidiInput::openDevice(inputIdentifier, this);
                if (midiOut && midiIn)
                {
                    midiIn->start();
                    setConnectionState(connectingToBootloader ? State::Bootloader : State::Connected);
                }
                else
                {
                    setConnectionState(State::Unavailable);
                }
            }

            return isConnected();
        }

        // ------------------------------------------------------------------------

        void disconnect()
        {
            if (midiIn)
            {
                midiIn->stop();
            }
            midiIn = nullptr;
            midiOut = nullptr;
            setConnectionState(State::Unavailable);
        }

        // ------------------------------------------------------------------------

        void sendMessage(const juce::MidiMessage& message)
        {
            if (midiOut)
            {
                midiOut->sendMessageNow(message);
            }
        }

        // ------------------------------------------------------------------------

        void handleIncomingMidiMessage(juce::MidiInput* /*source*/, const juce::MidiMessage& message) override
        {
            if (autoDisconnect)
            {
                startTimer(0, TimeoutMilliseconds);
            }

            if (message.isSysEx())
            {
                const uint8_t* m = message.getSysExData();
                const size_t s = message.getSysExDataSize();
                handleSysEx(m, s);
            }
            else
            {
                handleMidi(message);
            }
        }

        // ------------------------------------------------------------------------

        void timerCallback(int timerID) override
        {
            if (timerID != 0) return;

            if (connectionState == State::Connected)
            {
                // hit this timer because data flow has stopped
                if (autoDisconnect)
                {
                    disconnect();
                }
            }
            else if (canConnect())
            {
                if (!isConnected())
                {
                    if (autoReconnect)
                    {
                        connect();
                    }
                    else
                    {
                        setConnectionState(State::Available);
                    }
                }
            }
            else
            {
                setConnectionState(State::Unavailable);
            }
        }

        // ------------------------------------------------------------------------

    protected:
        std::unique_ptr<juce::MidiOutput> midiOut;
        std::unique_ptr<juce::MidiInput> midiIn;
        juce::String device, bootloader;
        State connectionState;
        bool autoReconnect, autoDisconnect;

        // ------------------------------------------------------------------------

        virtual void handleSysEx(const uint8_t* /*data*/, const size_t /*numBytes*/) {}
        virtual void handleMidi(const juce::MidiMessage& /*message*/) {}
        virtual void connectionStateChanged() {}
        
        // ------------------------------------------------------------------------

        void setConnectionState(State newState)
        {
            if (newState != connectionState)
            {
                connectionState = newState;
                connectionStateChanged();
            }
        }
        
        // ------------------------------------------------------------------------
        
        juce::String findIdentifierInMidiInfo(const juce::Array<juce::MidiDeviceInfo>& mdInfo, juce::String deviceName) const
        {
            const int size = mdInfo.size();
            int index = 0;
            while ((index < size) && (mdInfo[index].name != deviceName))
            {
                index++;
            }
            return (index == size) ? juce::String() : mdInfo[index].identifier;
        }

        // ------------------------------------------------------------------------

        void getIdentifiers(bool& wouldConnectToBootloader, juce::String& outputIdentifier, juce::String& inputIdentifier) const
        {
            const juce::Array<juce::MidiDeviceInfo>& outInfo = juce::MidiOutput::getAvailableDevices();
            outputIdentifier = findIdentifierInMidiInfo(outInfo, device);
            if (outputIdentifier.isNotEmpty())
            {
                wouldConnectToBootloader = false;
            }
            else
            {
                outputIdentifier = findIdentifierInMidiInfo(outInfo, bootloader);
                wouldConnectToBootloader = true;
            }

            inputIdentifier = juce::String();
            if (outputIdentifier.isNotEmpty())
            {
                const juce::Array<juce::MidiDeviceInfo>& inInfo = juce::MidiInput::getAvailableDevices();
                if (wouldConnectToBootloader)
                {
                    inputIdentifier = findIdentifierInMidiInfo(inInfo, bootloader);
                }
                else
                {
                    inputIdentifier = findIdentifierInMidiInfo(inInfo, device);
                }
            }
        }

        // ------------------------------------------------------------------------

    private:
        static constexpr int TimeoutMilliseconds = 600;
    };
};

/*
 * MIDI drivers
 * Container for JUCE MIDI input and output objects,
 * so MIDI traffic can more easily be administered in both directions
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace Midi
{
    enum class State { Unavailable, Available, Bootloader, Connected };
    enum class Connection { AsBootloader, AsDevice, AsEither };

    class MidiDuplex : public juce::MidiInputCallback, protected juce::Timer
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
            // this timer doesn't ever stop:
            // it handles unavailable/available signalling even when automatic modes
            // are switched off.
            startTimer(TimeoutMilliseconds);
        }

        // ------------------------------------------------------------------------

        ~MidiDuplex()
        {
            disconnect();
        }

        // ------------------------------------------------------------------------

        bool canConnect(const Connection option = Connection::AsEither) const
        {
            int outputIndex, inputIndex;
            bool wouldConnectToBootloader;
            getIndexes(wouldConnectToBootloader, outputIndex, inputIndex);
            if ((option == Connection::AsDevice) && wouldConnectToBootloader)
            {
                return false;
            }
            if ((option == Connection::AsBootloader) && !wouldConnectToBootloader)
            {
                return false;
            }
            return (outputIndex >= 0) && (inputIndex >= 0);
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
            startTimer(TimeoutMilliseconds);
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
            startTimer(TimeoutMilliseconds);
        }

        // ------------------------------------------------------------------------

        bool connect()
        {
            int outputIndex, inputIndex;
            bool connectingToBootloader;
            getIndexes(connectingToBootloader, outputIndex, inputIndex);
            disconnect();
            
            if ((outputIndex >= 0) && (inputIndex >= 0))
            {
                midiOut = juce::MidiOutput::openDevice(outputIndex);
                midiIn  = juce::MidiInput::openDevice(inputIndex, this);
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

        void handleIncomingMidiMessage(juce::MidiInput* /*source*/, const juce::MidiMessage& message)
        {
            if (autoDisconnect)
            {
                startTimer(TimeoutMilliseconds);
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

        void timerCallback()
        {
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

        virtual void handleSysEx(const uint8_t* /*buffer*/, const size_t /*numBytes*/) {}
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
        
        void getIndexes(bool& wouldConnectToBootloader, int& outputIndex, int& inputIndex) const
        {
            juce::StringArray sa;
            int index;
            sa = juce::MidiOutput::getDevices();
            index = sa.indexOf(device);
            if (index >= 0)
            {
                outputIndex = index;
                wouldConnectToBootloader = false;
            }
            else
            {
                index = sa.indexOf(bootloader);
                if (index >= 0)
                {
                    outputIndex = index;
                    wouldConnectToBootloader = true;
                }
                else
                {
                    outputIndex = -1;
                }
            }

            if (outputIndex >= 0)
            {
                sa = juce::MidiInput::getDevices();
                if (wouldConnectToBootloader)
                {
                    inputIndex = sa.indexOf(bootloader);
                }
                else
                {
                    inputIndex = sa.indexOf(device);
                }
            }
        }

    private:
        static constexpr int TimeoutMilliseconds = 600;
    };
};

/*
 * MIDI drivers
 * Manages input and output objects,
 * to allow simple MIDI control of a device in both directions.
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

#ifdef _WIN32
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <mmsystem.h>
 #include <memory>
 #include <mutex>
 #include <thread>
#endif

namespace Midi
{
    enum class State { Unavailable, Available, Bootloader, Connected };
    enum class Connection { AsBootloader, AsDevice, AsEither };

    class MidiDuplex : public juce::MidiInputCallback, protected juce::MultiTimer
    {
    public:
        MidiDuplex(const juce::String& deviceName, const juce::String& bootloaderName) :
            midiOut(nullptr),
            midiIn(nullptr),
            device(deviceName),
            bootloader(bootloaderName),
            connectionState(State::Unavailable),
            autoReconnect(false),
            autoDisconnect(true),
            lastMessageTime(0)
        {
            /* This timer doesn't ever stop: it handles unavailable/available signalling
               even when automatic modes are switched off.
               MultiTimer is used here because it's often necessary to use other timers
               in inherited classes. */
            startServiceProbe();
            startTimer(0, PollMilliseconds);
        }

        // ------------------------------------------------------------------------

        ~MidiDuplex()
        {
            /* Stop the timer before anything else, so a tick can't arrive while the
               derived part of the object is already gone. Derived classes that start
               timers of their own must stop them in their own destructors.
               closePorts() rather than disconnect(): a state-change callback during
               destruction would only ever reach the (empty) base implementation. */
            stopTimer(0);
            closePorts();
        }

        // ------------------------------------------------------------------------

        bool canConnect(Connection option = Connection::AsEither) const
        {
            if (!midiServiceIsResponsive()) return false;

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
        void setAutoReconnect(bool automaticReconnect)
        {
            // reconnects when the connection drops
            autoReconnect = automaticReconnect;
            startTimer(0, PollMilliseconds);
        }
        
        // ------------------------------------------------------------------------

        /** If this is set to true, the device will be marked as disconnected if
            traffic stops for the length of TimeoutMilliseconds. For this to work,
            the device would need either active sensing, or a guaranteed frequency
            of traffic. */
        void setAutoDisconnect(bool automaticDisconnect)
        {
            // disconnects when inbound traffic stops
            autoDisconnect = automaticDisconnect;
            startTimer(0, PollMilliseconds);
        }

        // ------------------------------------------------------------------------

        bool connect()
        {
            if (!midiServiceIsResponsive()) return false;

            juce::String outputIdentifier, inputIdentifier;
            bool connectingToBootloader = false;
            getIdentifiers(connectingToBootloader, outputIdentifier, inputIdentifier);

            /* closePorts() rather than disconnect(): disconnect() would announce
               Unavailable to listeners on the way to reconnecting, which reads as the
               device having been unplugged and plugged back in. */
            closePorts();

            if (outputIdentifier.isNotEmpty() && inputIdentifier.isNotEmpty() &&
                claimPorts(outputIdentifier, inputIdentifier))
            {
                midiOut = juce::MidiOutput::openDevice(outputIdentifier);
                midiIn  = juce::MidiInput::openDevice(inputIdentifier, this);
                if (midiOut && midiIn)
                {
                    openOutputIdentifier = outputIdentifier;
                    openInputIdentifier = inputIdentifier;
                    lastMessageTime = juce::Time::getMillisecondCounter();
                    midiIn->start();
                    setConnectionState(connectingToBootloader ? State::Bootloader : State::Connected);
                }
                else
                {
                    // half of a pair is no use to us, and holding it keeps the port
                    // away from other MIDI clients
                    closePorts();
                    setConnectionState(State::Unavailable);
                }
            }
            else
            {
                setConnectionState(State::Unavailable);
            }

            return isConnected();
        }

        // ------------------------------------------------------------------------

        void disconnect()
        {
            closePorts();
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
            /* Note the arrival time rather than restarting the timer: this runs on the
               MIDI thread, at up to 100Hz, and startTimer() would take the shared
               TimerThread lock every time. */
            lastMessageTime = juce::Time::getMillisecondCounter();

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

            if (!midiServiceIsResponsive())
            {
                // leave the connection state alone rather than blocking the message
                // thread inside the service, and look again on the next tick
                return;
            }

            if (isConnected())
            {
                /* Close the ports, rather than just reporting Unavailable, whenever the
                   connection has ended. Two things end it: the device going away (which
                   covers a Bootloader connection, and works whether or not autoDisconnect
                   is set), and inbound traffic stopping. */
                if (!openPortsAreStillPresent())
                {
                    disconnect();
                }
                else if (autoDisconnect && trafficHasStopped())
                {
                    disconnect();
                }
            }
            else if (canConnect())
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
            else
            {
                setConnectionState(State::Unavailable);
            }

            startServiceProbe();
        }

        // ------------------------------------------------------------------------

    protected:
        std::unique_ptr<juce::MidiOutput> midiOut;
        std::unique_ptr<juce::MidiInput> midiIn;
        juce::String device, bootloader;
        /* connectionState is written on the message thread and read from the MIDI
           thread (and from client code on any thread), so it can't be a plain enum. */
        std::atomic<State> connectionState;
        bool autoReconnect, autoDisconnect;

        // ------------------------------------------------------------------------

        virtual void handleSysEx(const uint8_t* /*data*/, size_t /*numBytes*/) {}
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

        /** Releases both ports without touching the connection state. */
        void closePorts()
        {
            if (midiIn)
            {
                midiIn->stop();
            }
            midiIn = nullptr;
            midiOut = nullptr;
            openOutputIdentifier = juce::String();
            openInputIdentifier = juce::String();
            releasePorts();
        }

        // ------------------------------------------------------------------------

        /** True while the two ports we actually hold open are still on the system's
            device lists. Checking the identifiers, rather than just the device names,
            also catches a device that has rebooted into its bootloader underneath us. */
        bool openPortsAreStillPresent() const
        {
            if (openOutputIdentifier.isEmpty() || openInputIdentifier.isEmpty())
            {
                return false;
            }
            return identifierIsPresent(juce::MidiOutput::getAvailableDevices(), openOutputIdentifier) &&
                   identifierIsPresent(juce::MidiInput::getAvailableDevices(), openInputIdentifier);
        }

        // ------------------------------------------------------------------------

        static bool identifierIsPresent(const juce::Array<juce::MidiDeviceInfo>& mdInfo,
                                        const juce::String& identifier)
        {
            for (const auto& info : mdInfo)
            {
                if (info.identifier == identifier) return true;
            }
            return false;
        }

        // ------------------------------------------------------------------------

        bool trafficHasStopped() const
        {
            // unsigned arithmetic, so this stays correct across a counter wrap
            return (juce::Time::getMillisecondCounter() - lastMessageTime) >=
                   static_cast<juce::uint32>(TimeoutMilliseconds);
        }

        // ------------------------------------------------------------------------

        /* Windows will happily let two processes open the same MIDI port, and then
           behave badly. A named mutex per port keeps Supperware clients out of each
           other's way: several plug-in instances, or a plug-in alongside Bridgehead.
           The mutex names are shared with MidiScanner and must not be changed. */
        bool claimPorts(const juce::String& outputIdentifier, const juce::String& inputIdentifier)
        {
#ifdef _WIN32
            if (claimPort(outputPortMutex, L"Global\\SuperwareMIDIOut_", outputIdentifier) &&
                claimPort(inputPortMutex, L"Global\\SuperwareMIDIIn_", inputIdentifier))
            {
                return true;
            }
            // another client has one of the two: don't hold half a pair
            releasePorts();
            return false;
#else
            juce::ignoreUnused(outputIdentifier, inputIdentifier);
            return true;
#endif
        }

        // ------------------------------------------------------------------------

        void releasePorts()
        {
#ifdef _WIN32
            releasePort(outputPortMutex);
            releasePort(inputPortMutex);
#endif
        }

        // ------------------------------------------------------------------------

#ifdef _WIN32
        static bool claimPort(HANDLE& handle, const wchar_t* prefix, const juce::String& identifier)
        {
            handle = nullptr;
            const juce::String mutexName = juce::String(prefix) +
                juce::String::toHexString(identifier.hashCode64());

            HANDLE h = CreateMutexW(NULL, FALSE, mutexName.toWideCharPointer());
            if (h == nullptr)
            {
                /* We can't arbitrate -- most likely the global namespace is off limits
                   to this process. Carry on unguarded rather than refusing to connect
                   at all, which is what the pre-mutex code did anyway. */
                return true;
            }

            const DWORD result = WaitForSingleObject(h, ContentionTimeoutMs);
            /* WAIT_ABANDONED means the previous owner died still holding it, so it is
               ours now. Treating that as a failure would leave the port permanently
               unclaimable after any client crash. */
            if ((result != WAIT_OBJECT_0) && (result != WAIT_ABANDONED))
            {
                CloseHandle(h);
                return false;
            }

            handle = h;
            return true;
        }

        // ------------------------------------------------------------------------

        static void releasePort(HANDLE& handle)
        {
            if (handle != nullptr)
            {
                ReleaseMutex(handle);
                CloseHandle(handle);
                handle = nullptr;
            }
        }
#endif

        // ------------------------------------------------------------------------

        /* Windows 11 routes winmm through wdmaud2 into the midisrv service, and that
           service can deadlock internally -- after which every MIDI enumeration call in
           every process blocks forever. JUCE requires enumeration to happen on the
           message thread, so a wedged service would freeze the host's UI outright.
           Instead we ask the service the same question from a throwaway thread, and
           skip the scan while it fails to answer. A healthy service replies in
           microseconds, so the poll interval is a generous timeout. */

        void startServiceProbe()
        {
#ifdef _WIN32
            if (probeState->outstanding.exchange(true)) return;  // one at a time

            /* A stranded probe can outlive this binary: if a host unloaded the plug-in
               while one was stuck, it would return into unmapped code. Pin the module
               so that can't happen. Costs us an unload; no-op in an application. */
            static std::once_flag pinned;
            std::call_once(pinned, []
            {
                static char anchor = 0;
                HMODULE thisModule = nullptr;
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_PIN,
                                   reinterpret_cast<LPCWSTR>(&anchor), &thisModule);
            });

            std::thread([state = probeState]
            {
                ::midiInGetNumDevs();       // the exact call that wedges
                state->outstanding.store(false);
            }).detach();
#endif
        }

        // ------------------------------------------------------------------------

        bool midiServiceIsResponsive() const
        {
#ifdef _WIN32
            return !probeState->outstanding.load();
#else
            return true;
#endif
        }
        
        // ------------------------------------------------------------------------
        
        juce::String findIdentifierInMidiInfo(const juce::Array<juce::MidiDeviceInfo>& mdInfo, juce::String deviceName) const
        {
            const int size = mdInfo.size();
            int index = 0;
            while ((index < size) && !(mdInfo[index].name.startsWith(deviceName)))
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
        /* Traffic has to stop for TimeoutMilliseconds before autoDisconnect fires; the
           timer polls more often than that so the delay stays close to the timeout. */
        static constexpr int TimeoutMilliseconds = 600;
        static constexpr int PollMilliseconds = 300;

        juce::String openOutputIdentifier, openInputIdentifier;
        std::atomic<juce::uint32> lastMessageTime;

#ifdef _WIN32
        static constexpr int ContentionTimeoutMs = 20;
        HANDLE inputPortMutex = nullptr;
        HANDLE outputPortMutex = nullptr;

        // held by shared_ptr so that a probe stranded inside a wedged service can
        // safely outlive this object
        struct ServiceProbe { std::atomic<bool> outstanding { false }; };
        std::shared_ptr<ServiceProbe> probeState { std::make_shared<ServiceProbe>() };
#endif
    };
};

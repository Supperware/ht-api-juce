# ht-api-juce

# Head Tracker API for C++/JUCE

This API adds a straightforward interfacing/visualising/configuring panel to your JUCE app or plug-in.

## Why you might want to use this

There are two easy ways of making your app or plug-in compatible with the head tracker. One is to use this code, which looks after the boring stuff and throws you a 3D rotation matrix whenever there's a new frame of data. The other is to remind your users about Supperware's [Bridgehead app](https://supperware.co.uk/headtracker), and support a sensible OSC convention, such as `\quaternion` or `\yaw` `\pitch` `\roll`.

### Reasons to use this API
    
1. Your software is seamlessly integrated with the head tracker. Your users can just 'click the tick' to connect rather than fiddling with the bones of OSC, and the animated head and basic configuration are built into your program.
2. It doesn't involve loading Bridgehead first.
3. Latency will be lower than using OSC, as unmediated messages come straight from the head tracker.
4. You don't mind including JUCE in your project, or its dependencies (including OpenGL), although see the note below about not using JUCE if you don't want to ...

### Reasons to use OSC via Bridgehead instead

1. Windows is not multi-MIDI-client. If you expect people to have multiple instantiations of head-tracked apps or plug-ins open, only one will be able to interface with the head tracker's MIDI stream at any time. (If you click the tick and it won't turn green, this is generally why.) macOS does not have this limitation, and Bridgehead allows OSC to be cast to several ports at once.
2. In your case, tightness of integration might be secondary to allowing users to remix or manipulate the head tracker data. Such users might prefer to work with OSC.
3. You won't have to download this code, or work with C++ or JUCE.

### Using this API without JUCE

JUCE provides cross-platform libraries for MIDI and graphics. If you'd rather not use it, you don't have to start from scratch. The following header files do not require JUCE, and will compile with just the standard libraries:

- `supperware/HeadMatrix.h` transforms orientation data from the head tracker (either yaw/pitch/roll or quaternions) into a double-buffered 3D rotation matrix. This may be used directly to perform world-to-head or head-to-world rotations.
- `supperware/Tracker.h` is a helper class. It builds appropriate outgoing MIDI messages, interprets incoming MIDI messages and routes them to appropriate callbacks, and maintains a copy of the current configuration state of the head tracker. To see how this is wrapped in JUCE, take a look at `supperware/midi/midi-TrackerDriver.h`.

### The third way, and a bit about Bridgehead

If none of this is what you need, you may have to write your own MIDI interface code from scratch: the [support page](https://supperware.co.uk/headtracker) contains detailed MIDI documentation.

That page also links to getting-started examples in other languages, mostly provided by customers: you may find something there that you prefer to begin with.

Even if you want to use the API for everything, please still download Bridgehead and run it periodically. One thing that Bridgehead will do that the API does not is upgrade the head tracker firmware. Anything that complicated is going to have bugs!

## How to get this demo running

If you've not used JUCE before, you should start by downloading it [here](https://github.com/juce-framework/JUCE). The workflow is then the traditional JUCE one. Build the Projucer, use it to open `demo/demo.jucer`, and point the Projucer to your JUCE library. You can then generate the appropriate project file, and open and build it in your usual SDK.

In use, plug in a head tracker. A tick will become visible in the bottom left corner of the head tracker panel. Click on it once to connect to the head tracker. The tick will turn green, while a wireframe head appears and moves to show the current head orientation. Double-click on the head to zero the tracker. Click on the tick again to disconnect.

A configuration window can be opened by clicking on the pictogram of the head tracker in the top-left. This presents a handy but reduced subset of the functions you would find if you were using _Bridgehead_.

You probably don't care whether you're interfacing with the head tracker via quaternions or yaw, pitch, and roll. While the head tracker and API supports both (search for `trackerDriver.turnOn` in `supperware/headpanel/headpanel-Component.h`), it's recommended to keep using quaternions unless you have a great reason not to, as you won't risk gimbal lock. That said, gimbal lock is mostly a problem in theory. First, yaw/pitch/roll will go awry when a user's head is pitched nearly fully skywards or downwards, and generally people don't enjoy those contortions. Second, everything is manipulated as orthonormal matrices inside the head tracker anyway so it's not going to lead to internal state chaos.

## Notes from users

The driver will disconnect if no data is received from the head tracker after a few hundred milliseconds. This feature is included because some operating systems won't let you know if a MIDI device you're talking to is unplugged mid-conversation. Usually the head tracker is sending data at 25Hz or more when it is connected and turned on, but if you are experimenting or using breakpoints in certain ways it is possible to hit this timeout. It can be disabled using two lines of code in MainComponent.cpp:

```
Midi::TrackerDriver td = headPanel.getTrackerDriver();
td.setAutoDisconnect(false); // from the MidiDuplex class
```

but you may want to leave autoDisconnect on when you're ready to deploy for the reasons stated above.

## Licensing

See the `LICENSE` file in the supperware folder! The API code is released under the MIT License. The `demo` app is based around JUCE boilerplate code with a handful of extra lines to show you how to get the panel working, and you can use this without restriction.

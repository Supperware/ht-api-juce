# ht-api-juce

# Head Tracker API for C++/JUCE

This API adds a straightforward interfacing/visualising/configuring panel to your JUCE app or plug-in.

## Why you might want to use this

There are two easy ways of making your app or plug-in compatible with the head tracker. One is to use this code, which looks after the boring stuff and throws you a 3D rotation matrix whenever there's a new frame of data. The other is to remind your users about Supperware's [Bridgehead app](https://supperware.co.uk/headtracker), and support a sensible OSC convention, such as `\quaternion` or `\yaw` `\pitch` `\roll`.

### Reasons to use this API
    
1. Your software is seamlessly integrated with the head tracker. Your users can just 'click the tick' to connect rather than fiddling with the bones of OSC, and the animated head and basic configuration are built into your program.
2. It doesn't involve loading Bridgehead first.
3. Latency will be lower than using OSC, as unmediated messages come straight from the head tracker.

### Reasons to use OSC via Bridgehead instead

1. Windows is not multi-MIDI-client. If you expect people to have multiple instantiations of head-tracked apps or plug-ins open, only one will be able to interface with the head tracker's MIDI stream at any time. (If you click the tick and it won't turn green, this is generally why.) macOS does not have this limitation, and Bridgehead allows OSC to be cast to several ports at once.
2. In your case, tightness of integration might be secondary to allowing users to remix or manipulate the head tracker data. Such users might prefer to work with OSC.
3. You won't have to download this code, or work with C++ or JUCE.

### The third way, and a bit about Bridgehead

An alternative to using the full API is to use part of it, or just to write your own MIDI interfacing code from scratch: the [support page](https://supperware.co.uk/headtracker) contains detailed MIDI documentation.

Our support page also links to some getting-started examples in other languages, mostly provided by customers, and you may find something there that you prefer to begin with.

Even if you use the API, please still download Bridgehead and run it periodically. One thing that Bridgehead will do that the API does not is upgrade the head tracker firmware. Anything that complicated is going to have bugs!

## How to get this demo running

If you've not used JUCE before, you should start by downloading it [here](https://github.com/juce-framework/JUCE). The workflow is then the traditional JUCE one. Build the Projucer, use it to open `demo\demo.jucer`, and point the Projucer to your JUCE library. You can then generate the appropriate project file, and open and build it in your usual SDK.

In use, plug in a head tracker. A tick will become visible in the bottom left corner of the head tracker panel. Click on it once to connect to the head tracker. The tick will turn green, while a wireframe head appears and moves to show the current head orientation. Double-click on the head to zero the tracker. Click on the tick again to disconnect.

A configuration window can be opened by clicking on the pictogram of the head tracker in the top-left. This presents a handy but reduced subset of the functions you would find if you were using _Bridgehead_.

You probably don't care whether you're interfacing with the head tracker via quaternions or yaw, pitch, and roll. While the head tracker and API supports both (search for `tracker.turnOn` in `headpanel-Component.h`), it's recommended to keep using quaternions unless you have a great reasson not to. In head tracking, gimbal lock is mostly a problem in theory only: yaw/pitch/roll will start to go slightly awry when a user's head is pitched fully skywards or downwards, and generally people don't enjoy those contortions. But, if you use quaternions, you won't have to worry about them at all.

## Licensing

See the `LICENSE` file in the headtracker folder! The API code is released under the MIT License. The `demo` app is based around JUCE boilerplate code with a handful of extra lines to show you how to get the panel working, and you can use this without restriction.

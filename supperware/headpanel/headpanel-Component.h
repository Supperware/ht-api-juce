/*
 * Head tracker panel and driver API
 * Animated head and configuration buttons
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace HeadPanel
{
    /** Component that manages head tracker settings, disconnection/reconnection, and shows 
      * instantaneous head angle. Also owns Midi::Tracker and SBR::HeadMatrix objects, which
      * are useful everywhere else. */
    class HeadPanel: public juce::Component, juce::Timer, Midi::TrackerDriver::Listener, HeadButton::Listener

    {
    public:
        class Listener
        {
        public:
            virtual ~Listener() {};
            virtual void trackerChanged(const HeadMatrix& headMatrix) = 0;
        };

        HeadPanel() :
            listener(nullptr),
            settingsPanel(trackerDriver),
            hbConfigure(this, 0),
            hbConnect(this, 1),
            doRepaint(false),
            gazeInitial(0),
            gazeNow(0),
            midiState(Midi::State::Unavailable)
        {
            juce::MemoryInputStream mis(BinaryData::mini_tile_png, BinaryData::mini_tile_pngSize, false);
            juce::Image im = juce::ImageFileFormat::loadFrom(mis);

            trackerDriver.addListener(this);
            setSize(148, 104);
            doButton(hbConfigure, im, 0, 2, 6);
            doButton(hbConnect, im, 1, 2, 58);
            hbConnect.setVisible(false);
        }

        //----------------------------------------------------------------------

        const Midi::TrackerDriver& getTrackerDriver() const
        {
            return trackerDriver;
        }

        //----------------------------------------------------------------------

        const HeadMatrix& getHeadMatrix() const
        {
            return headMatrix;
        }

        //----------------------------------------------------------------------

        void paint(juce::Graphics& g) override
        {
            //g.setColour(Colour(0xff404040));
            //g.drawRect(g.getClipBounds(), 1.0f);
            plot.paint(g, 48 + 50, 48 + 4, 48.0f, 2.0f, midiState);
        }

        //----------------------------------------------------------------------

        void trackerOrientation(float yawRadian, float pitchRadian, float rollRadian) override
        {
            headMatrix.setOrientationYPR(yawRadian, pitchRadian, rollRadian);
            plot.recalculate(headMatrix);
            if (listener) listener->trackerChanged(headMatrix);
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void trackerOrientationQ(float qw, float qx, float qy, float qz) override
        {
            headMatrix.setOrientationQuaternion(qw, qx, qy, qz);
            plot.recalculate(headMatrix);
            if (listener) listener->trackerChanged(headMatrix);
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void trackerMidiConnectionChanged(const Midi::State& newState) override
        {
            if (newState != midiState)
            {
                midiState = newState;

                if ((midiState == Midi::State::Connected) || (midiState == Midi::State::Bootloader))
                {
                    hbConnect.setVisible(true);
                    hbConnect.setSelected(true);
                }
                else if (midiState == Midi::State::Available)
                {
                    hbConnect.setVisible(true);
                    hbConnect.setSelected(false);
                    headMatrix.zero();
                }
                else // Unavailable
                {
                    hbConnect.setVisible(false);
                    headMatrix.zero();
                }
                if (listener) listener->trackerChanged(headMatrix);
                flagRepaint();
            }
        }

        //----------------------------------------------------------------------

        void trackerConnectionChanged(const Tracker::State& state) override
        {
            settingsPanel.trackUpdatedState(state.rightEarChirality, state.compassOn, state.travelMode);
        }

        //----------------------------------------------------------------------

        void mouseDown(const juce::MouseEvent& /*event*/) override
        {
            gazeInitial = gazeNow;
        }

        // -------------------------------------------------------------------------

        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (midiState == Midi::State::Connected)
            {
                int s = event.getDistanceFromDragStartY();
                gazeNow = gazeInitial - (s / 2.0f);
                if (gazeNow > 90.0f) gazeNow = 90.0f;
                if (gazeNow < 0.0f)  gazeNow = 0.0f;
                plot.setGazeAngle(gazeNow);
                flagRepaint();
            }
        }

        // -------------------------------------------------------------------------

        void mouseDoubleClick(const juce::MouseEvent& /*event*/) override
        {
            trackerDriver.zero();
        }

        //----------------------------------------------------------------------

        void headButtonSelect(int index) override
        {
            if (!index)
            {
                // settings button
                juce::DialogWindow::LaunchOptions lo;
                lo.content.set(&settingsPanel, false);
                lo.dialogBackgroundColour = juce::Colour(0xff181818);
                lo.dialogTitle = "Head Tracker Settings";
                lo.escapeKeyTriggersCloseButton = true;
                lo.resizable = false;
                lo.useNativeTitleBar = true;
                lo.componentToCentreAround = this;
                lo.launchAsync();
            }
            else if (index == 1)
            {
                // connect/disconnect button
                if (midiState == Midi::State::Available)
                {
                    trackerDriver.connect();
                    trackerDriver.turnOn(false, true);
                }
                else
                {
                    trackerDriver.disconnect();
                }
            }
        }

        //----------------------------------------------------------------------

        void doButton(HeadButton& b, juce::Image& masterImage, int imageIndex, int x, int y)
        {
            double dpi = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi;
            juce::Image subImage;
            if (dpi > 128.0)
            {
                subImage = masterImage.getClippedImage(juce::Rectangle<int>(40 + 120 * imageIndex, 0, 120, 120));
            }
            else
            {
                subImage = masterImage.getClippedImage(juce::Rectangle<int>(0, 40 * imageIndex, 40, 40));
            }

            b.setTopLeftPosition(x, y);
            b.setImage(subImage);
            addAndMakeVisible(b);
        }

        //----------------------------------------------------------- ----------

        void timerCallback() override
        {
            stopTimer();
            if (doRepaint)
            {
                juce::MessageManagerLock mml;
                doRepaint = false;
                repaint();
            }
        }

        //----------------------------------------------------------- ----------

        void setListener(Listener* l)
        {
            listener = l;
        }

    private:
        Listener* listener;
        Midi::TrackerDriver trackerDriver;
        HeadMatrix headMatrix;
        ConfigPanel::SettingsPanel settingsPanel;

        HeadButton hbConfigure, hbConnect;
        HeadPlot plot;
        bool doRepaint;
        float gazeInitial, gazeNow;

        Midi::State midiState;

        //----------------------------------------------------------- ----------

        void flagRepaint()
        {
            if (!doRepaint)
            {
                doRepaint = true;
                startTimer(20);
            }
        }
    };
};

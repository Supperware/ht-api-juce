/*
 * Head tracker panel and driver API
 * Animated head and configuration buttons
 * Copyright 2021 Supperware Ltd.
 */

#pragma once

namespace HeadPanel
{
    /** Component that manages head tracker settings, disconnection/reconnection, and shows 
      * instantaneous head angle. Also owns Midi::Tracker and SBR::HeadMatrix objects, which
      * are useful everywhere else. */
    class HeadPanel: public Component, Timer, Midi::Tracker::Receiver, HeadButton::Listener

    {
    public:
        class Receiver
        {
        public:
            virtual ~Receiver() {};
            virtual void trackerChanged(const HeadMatrix& headMatrix) = 0;
        };

        HeadPanel() :
            receiver(nullptr),
            tracker(this),
            headMatrix(),
            settingsPanel(tracker),
            hbConfigure(this, 0),
            hbConnect(this, 1),
            plot(),
            doRepaint(false),
            state(Midi::State::Unavailable)
        {
            MemoryInputStream mis(BinaryData::mini_tile_png, BinaryData::mini_tile_pngSize, false);
            Image im = ImageFileFormat::loadFrom(mis);

            setSize(148, 104);
            doButton(hbConfigure, im, 0, 2, 6);
            doButton(hbConnect, im, 1, 2, 58);
            hbConnect.setVisible(false);
        }

        //----------------------------------------------------------------------

        const Midi::Tracker& getTracker() const
        {
            return tracker;
        }

        //----------------------------------------------------------------------

        const HeadMatrix& getHeadMatrix() const
        {
            return headMatrix;
        }

        //----------------------------------------------------------------------

        void paint(Graphics& g) override
        {
            //g.setColour(Colour(0xff404040));
            //g.drawRect(g.getClipBounds(), 1.0f);
            plot.paint(g, 48 + 50, 48 + 4, 48.0f, 2.0f, state);
        }

        //----------------------------------------------------------------------

        void trackOrientation(float yawRadian, float pitchRadian, float rollRadian) override
        {
            headMatrix.setOrientationYPR(yawRadian, pitchRadian, rollRadian);
            plot.recalculate(headMatrix);
            if (receiver) receiver->trackerChanged(headMatrix);
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void trackOrientationQ(float qw, float qx, float qy, float qz) override
        {
            headMatrix.setOrientationQuaternion(qw, qx, qy, qz);
            plot.recalculate(headMatrix);
            if (receiver) receiver->trackerChanged(headMatrix);
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void trackConnectionState(Midi::State newState) override
        {
            if (newState != state)
            {
                state = newState;

                if ((state == Midi::State::Connected) || (state == Midi::State::Bootloader))
                {
                    hbConnect.setVisible(true);
                    hbConnect.setSelected(true);
                }
                else if (state == Midi::State::Available)
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
                settingsPanel.trackConnectionState(newState);
                if(receiver) receiver->trackerChanged(headMatrix);
                flagRepaint();
            }
        }

        //----------------------------------------------------------------------

        void trackCompassState(Midi::CompassState compassState) override
        {
            settingsPanel.trackCompassState(compassState);
        }

        //----------------------------------------------------------------------

        void trackUpdatedState(bool rightEarChirality, bool compassOn, Midi::TravelMode travelMode) override
        {
            settingsPanel.trackUpdatedState(rightEarChirality, compassOn, travelMode);
        }

        //----------------------------------------------------------------------

        void mouseDoubleClick(const MouseEvent& /*event*/) override
        {
            tracker.zero();
        }

        //----------------------------------------------------------------------

        void headButtonSelect(int index) override
        {
            if (!index)
            {
                // settings button
                DialogWindow::LaunchOptions lo;
                lo.content.set(&settingsPanel, false);
                lo.dialogBackgroundColour = Colour(0xff181818);
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
                if (state == Midi::State::Available)
                {
                    tracker.connect();
                    tracker.turnOn(false, Midi::AngleFormat::Quaternion);
                }
                else
                {
                    tracker.disconnect();
                }
            }
        }

        //----------------------------------------------------------------------

        void doButton(HeadButton& b, Image& masterImage, int imageIndex, int x, int y)
        {
            double dpi = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi;
            Image subImage;
            if (dpi > 128.0)
            {
                subImage = masterImage.getClippedImage(Rectangle<int>(40 + 120 * imageIndex, 0, 120, 120));
            }
            else
            {
                subImage = masterImage.getClippedImage(Rectangle<int>(0, 40 * imageIndex, 40, 40));
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
                MessageManagerLock mml;
                doRepaint = false;
                repaint();
            }
        }

        //----------------------------------------------------------- ----------

        void setReceiver(Receiver* r)
        {
            receiver = r;
        }

    private:
        Receiver* receiver;
        Midi::Tracker tracker;
        HeadMatrix headMatrix;
        ConfigPanel::SettingsPanel settingsPanel;

        HeadButton hbConfigure, hbConnect;
        HeadPlot plot;
        bool doRepaint;
        Midi::State state;

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

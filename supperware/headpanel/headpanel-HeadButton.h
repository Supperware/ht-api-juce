/*
 * Head tracker panel and driver API
 * A simple clickable, hoverable image button
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace HeadPanel
{
    class HeadButton: public juce::Component, private juce::Timer
    {
    public:
        class Listener
        {
        public:
            virtual ~Listener() {};
            virtual void headButtonSelect(int /*index*/) {};
        };

        HeadButton(Listener* listener, int index) :
            l(listener), im(), listenerIndex(index), isSelected(false), isHovering(false), doRepaint(false)
        {
            setSize(40, 40);
        }

        void paint(juce::Graphics& g) override
        {
            if (isHovering && !isSelected)
            {
                g.setColour(juce::Colour(0x20ffffff));
                g.fillRect(g.getClipBounds());
            }
            /**/ if (isSelected) { g.setColour(juce::Colours::white); }
            else if (isHovering) { g.setColour(juce::Colour(0x80ffffff)); }
            else                 { g.setColour(juce::Colour(0x40ffffff)); }
            g.drawImageWithin(im, 0, 0, 40, 40, juce::RectanglePlacement());
        }

        //----------------------------------------------------------------------

        void mouseEnter(const juce::MouseEvent& /*event*/) override
        {
            isHovering = true;
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void mouseExit(const juce::MouseEvent& /*event*/) override
        {
            isHovering = false;
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void mouseUp(const juce::MouseEvent& /*event*/) override
        {
            isHovering = false;
            flagRepaint();
        }

        //----------------------------------------------------------------------

        void mouseDown(const juce::MouseEvent& /*event*/) override
        {
            l->headButtonSelect(listenerIndex);
        }

        //----------------------------------------------------------------------

        void setImage(const juce::Image& image)
        {
            im = image;
            im.duplicateIfShared();
        }

        //----------------------------------------------------------------------

        void setSelected(const bool shouldBeSelected)
        {
            isSelected = shouldBeSelected;
            flagRepaint();
        }

        //----------------------------------------------------------------------

        bool getSelected() const
        {
            return isSelected;
        }

        //----------------------------------------------------------------------

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

        //----------------------------------------------------------------------

    private:
        Listener* l;
        juce::Image im;
        int listenerIndex;
        bool isSelected, isHovering, doRepaint;

        void flagRepaint()
        {
            doRepaint = true;
            startTimer(20);
        }
    };
};

/*
 * Head tracker configuration panels
 * Extends JUCE LookAndFeel to draw proper radio buttons
 * Copyright 2021 Supperware Ltd.
 */

#pragma once

namespace ConfigPanel
{
    class LookAndFeelRadio : public LookAndFeel_V4
    {
    public:
        ~LookAndFeelRadio() override {}

        void drawTickBox (Graphics& g, Component& component,
                          float x, float y, float w, float h,
                          const bool ticked,
                          const bool isEnabled,
                          const bool shouldDrawButtonAsHighlighted,
                          const bool shouldDrawButtonAsDown) override
        {
            ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

            Rectangle<float> tickBounds (x, y, w, h);

            g.setColour (component.findColour (ToggleButton::tickDisabledColourId));
            g.drawEllipse (tickBounds.reduced(0.5f, 0.5f), 1.0f);
    
            if (ticked)
            {
                g.setColour (component.findColour (ToggleButton::tickColourId));
                g.fillEllipse(tickBounds.reduced (5.5f, 5.5f).toFloat());
            }
        }
    };
};

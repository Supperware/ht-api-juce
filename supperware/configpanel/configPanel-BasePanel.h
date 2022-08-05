/*
 * Head tracker configuration panels
 * Handles common activities concerning configuration windows
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace ConfigPanel
{
    enum class LabelStyle { SectionHeading, Description, Data, SubData };

    class BasePanel : public juce::Component, public juce::MultiTimer,
        private juce::Button::Listener, private juce::ComboBox::Listener
    {
    public:
        static constexpr int LabelWidth { 256 }; // window width is derived from LabelWidth

        BasePanel(juce::String titleText) :
            Component(),
            labels(), textButtons(), toggleButtons(), comboBoxes(),
            title(titleText), lookAndFeelRadio()
        {
            setOpaque(false);
        }

        // ---------------------------------------------------------------------

        ~BasePanel()
        {
            for (int i = 0; i < toggleButtons.size(); ++i)
            {
                toggleButtons[i]->setLookAndFeel(nullptr);
            }
        }

        // ---------------------------------------------------------------------

        virtual void click(const bool /*isTextButton*/, const int /*index*/, const bool /*isChecked*/) {}
        virtual void comboBox(const int /*index*/, const int /*option*/) {}

        // ---------------------------------------------------------------------
        
        void buttonClicked(juce::Button* button) override
        {
            juce::TextButton* textB = dynamic_cast<juce::TextButton*>(button);
            if (textB)
            {
                click(true, textButtons.indexOf(textB), false);
            }
            else
            {
                juce::ToggleButton* toggleB = dynamic_cast<juce::ToggleButton*>(button);
                if (toggleB)
                {
                    click(false, toggleButtons.indexOf(toggleB), toggleB->getToggleState());
                }
            }
        }

        // ---------------------------------------------------------------------

        void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override
        {
            int cbIndex = comboBoxes.indexOf(comboBoxThatHasChanged);
            comboBox(cbIndex, comboBoxThatHasChanged->getSelectedItemIndex());
        }

        // ---------------------------------------------------------------------

        /** Unilaterally enable or disable all controls */
        void enablePanel(const bool shouldBeEnabled)
        {
            int i;
            for (i = 0; i < textButtons.size(); ++i)
            {
                textButtons[i]->setEnabled(shouldBeEnabled);
            }
            for (i = 0; i < toggleButtons.size(); ++i)
            {
                toggleButtons[i]->setEnabled(shouldBeEnabled);
            }
            for (i = 0; i < comboBoxes.size(); ++i)
            {
                comboBoxes[i]->setEnabled(shouldBeEnabled);
            }
        }

        void paint(juce::Graphics& g) override
        {
            if (!title.isEmpty())
            {
                // draw title
                g.setColour(juce::Colour(0xff0c0c0c));
                g.fillRect(0, 2, LabelWidth, 21);
                g.setColour(TitleLabel);
                g.setFont(juce::Font(16.0, juce::Font::plain));
                g.drawText(title, 8, 4, 200, 200, juce::Justification::topLeft);
            }
        }

        // ------------------------------------------------------------------------

protected:
        juce::OwnedArray<juce::Label> labels;
        juce::OwnedArray<juce::TextButton> textButtons;
        juce::OwnedArray<juce::ToggleButton> toggleButtons;
        juce::OwnedArray<juce::ComboBox> comboBoxes;

        // ------------------------------------------------------------------------

        int addLabel(juce::Point<int>& topLeft, const juce::String text, const LabelStyle style)
        {
            int styleIndex;
            switch (style)
            {
            case LabelStyle::SectionHeading: styleIndex = 0; break;
            case LabelStyle::Description: styleIndex = 1; break;
            case LabelStyle::Data: styleIndex = 2; break;
            default: styleIndex = 3;
            }

            const float LabelFontSize[4] = { 15.5f, 14.5f, 16.0f, 16.0f };
            const int LabelFontStyle[4] = { juce::Font::FontStyleFlags::bold, 0, 0, 0 };
            const juce::Colour LabelColour[4] = { SectionLabel, juce::Colour(0xff808186), juce::Colour(0xffd8d8d8), juce::Colour(0xffa4a5a8)};
            const int LineHeight[4] = { 28, 24, 24, 24 };

            int index = labels.size();
            juce::Label* lb = labels.add(new juce::Label(text, text));
            lb->setTopLeftPosition(topLeft);
            lb->setSize(LabelWidth, LineHeight[styleIndex]);
            lb->setFont(juce::Font(LabelFontSize[styleIndex], LabelFontStyle[styleIndex]));
            lb->setColour(juce::Label::ColourIds::textColourId, LabelColour[styleIndex]);
            topLeft.y += LineHeight[styleIndex] + 3;
            addAndMakeVisible(lb);
            return index;
        }

        // ------------------------------------------------------------------------

        int addTextButton(juce::Point<int>& topLeft, const juce::String text, const int width)
        {
            constexpr int ButtonHeight { 24 };
            int index = textButtons.size();
            juce::TextButton* tb = textButtons.add(new juce::TextButton(text));
            tb->setTopLeftPosition(topLeft.translated(Indent + 10, 2));
            tb->setSize(width, ButtonHeight);
            tb->addListener(this);
            topLeft.y += ButtonHeight + 8;
            addAndMakeVisible(tb);
            return index;
        }

        // ------------------------------------------------------------------------

        int addToggle(juce::Point<int>& topLeft, const juce::String text, const int radioGroupID = -1)
        {
            constexpr int ToggleHeight { 22 };
            int index = toggleButtons.size();
            juce::ToggleButton* tb = toggleButtons.add(new juce::ToggleButton(text));
            if (radioGroupID >= 0)
            {
                tb->setRadioGroupId(radioGroupID, juce::dontSendNotification);
                tb->setLookAndFeel(&lookAndFeelRadio);
            }
            tb->setTopLeftPosition(topLeft.translated(Indent, 0));
            tb->setSize(LabelWidth - 16, ToggleHeight);
            tb->addListener(this);
            topLeft.y += ToggleHeight + 4;
            addAndMakeVisible(tb);
            return index;
        }

        // ------------------------------------------------------------------------

        int addComboBox(juce::Point<int>& topLeft, const juce::StringArray& text)
        {
            constexpr int ComboBoxHeight { 24 };
            int index = comboBoxes.size();
            juce::ComboBox* cb = comboBoxes.add(new juce::ComboBox());
            cb->setEditableText(false);
            cb->addItemList(text, 1);
            cb->setTopLeftPosition(topLeft.translated(Indent - 4, 0));
            cb->setSize(LabelWidth - 2 * Indent - topLeft.x, ComboBoxHeight);
            cb->addListener(this);
            topLeft.y += ComboBoxHeight + 4;
            addAndMakeVisible(cb);
            return index;
        }

        // ------------------------------------------------------------------------

        void timerCallback(int timerID) override
        {
            if (timerID == 0)
            {
                repaint();
                stopTimer(timerID);
            }
        }

        // ------------------------------------------------------------------------

    protected:
        juce::String title;
        const juce::Colour TitleLabel = juce::Colour(0xff28789c);
        const juce::Colour SectionLabel = juce::Colour(0xff529aca);

        static constexpr int Indent { 18 };

        int yOrigin()
        {
            return (title.isEmpty()) ? 2 : 22;
        }

        void flagRepaint()
        {
            startTimer(0, 17); // 60Hz repaint
        }

        // ------------------------------------------------------------------------

    private:
        LookAndFeelRadio lookAndFeelRadio;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasePanel)
    };
};

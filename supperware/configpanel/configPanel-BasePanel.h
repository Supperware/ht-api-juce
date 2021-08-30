/*
 * Head tracker configuration panels
 * Handles common activities concerning configuration windows
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace ConfigPanel
{
    enum class LabelStyle { SectionHeading, Description, Data, SubData };

    class BasePanel : public Component, public MultiTimer,
        private Button::Listener, private ComboBox::Listener
    {
    public:
        static constexpr int LabelWidth { 256 }; // window width is derived from LabelWidth

        BasePanel(String titleText) :
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
        
        void buttonClicked(Button* button) override
        {
            TextButton* textB = dynamic_cast<TextButton*>(button);
            if (textB)
            {
                click(true, textButtons.indexOf(textB), false);
            }
            else
            {
                ToggleButton* toggleB = dynamic_cast<ToggleButton*>(button);
                if (toggleB)
                {
                    click(false, toggleButtons.indexOf(toggleB), toggleB->getToggleState());
                }
            }
        }

        // ---------------------------------------------------------------------

        void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
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

        void paint(Graphics& g) override
        {
            if (!title.isEmpty())
            {
                // draw title
                g.setColour(Colour(0xff0c0c0c));
                g.fillRect(0, 2, LabelWidth, 21);
                g.setColour(TitleLabel);
                g.setFont(Font(16.0, Font::plain));
                g.drawText(title, 8, 4, 200, 200, Justification::topLeft);
            }
        }

        // ------------------------------------------------------------------------

protected:
        OwnedArray<Label> labels;
        OwnedArray<TextButton> textButtons;
        OwnedArray<ToggleButton> toggleButtons;
        OwnedArray<ComboBox> comboBoxes;

        // ------------------------------------------------------------------------

        int addLabel(Point<int>& topLeft, const String text, const LabelStyle style)
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
            const int LabelFontStyle[4] = { Font::FontStyleFlags::bold, 0, 0, 0 };
            const Colour LabelColour[4] = { SectionLabel, Colour(0xff808186), Colour(0xffd8d8d8), Colour(0xffa4a5a8)};
            const int LineHeight[4] = { 28, 24, 24, 24 };

            int index = labels.size();
            Label* lb = labels.add(new Label(text, text));
            lb->setTopLeftPosition(topLeft);
            lb->setSize(LabelWidth, LineHeight[styleIndex]);
            lb->setFont(Font(LabelFontSize[styleIndex], LabelFontStyle[styleIndex]));
            lb->setColour(Label::ColourIds::textColourId, LabelColour[styleIndex]);
            topLeft.y += LineHeight[styleIndex] + 3;
            addAndMakeVisible(lb);
            return index;
        }

        // ------------------------------------------------------------------------

        int addTextButton(Point<int>& topLeft, const String text, const int width)
        {
            constexpr int ButtonHeight { 24 };
            int index = textButtons.size();
            TextButton* tb = textButtons.add(new TextButton(text));
            tb->setTopLeftPosition(topLeft.translated(Indent + 10, 2));
            tb->setSize(width, ButtonHeight);
            tb->addListener(this);
            topLeft.y += ButtonHeight + 8;
            addAndMakeVisible(tb);
            return index;
        }

        // ------------------------------------------------------------------------

        int addToggle(Point<int>& topLeft, const String text, const int radioGroupID = -1)
        {
            constexpr int ToggleHeight { 22 };
            int index = toggleButtons.size();
            ToggleButton* tb = toggleButtons.add(new ToggleButton(text));
            if (radioGroupID >= 0)
            {
                tb->setRadioGroupId(radioGroupID, dontSendNotification);
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

        int addComboBox(Point<int>& topLeft, const StringArray& text)
        {
            constexpr int ComboBoxHeight { 24 };
            int index = comboBoxes.size();
            ComboBox* cb = comboBoxes.add(new ComboBox());
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
        String title;
        const Colour TitleLabel = Colour(0xff28789c);
        const Colour SectionLabel = Colour(0xff529aca);

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

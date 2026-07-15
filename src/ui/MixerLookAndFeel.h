#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct MixerLookAndFeel final : juce::LookAndFeel_V4 {
    MixerLookAndFeel() : LookAndFeel_V4 (juce::LookAndFeel_V4::ColourScheme {
            juce::Colour{ 0xff6366f1 },   // windowBackground
            juce::Colour{ 0xff252830 },   // widgetBackground
            juce::Colour{ 0xff252830 },   // menuBackground
            juce::Colour{ 0xff363840 },   // outline
            juce::Colour{ 0xffe4e4e7 },   // defaultText
            juce::Colour{ 0xff252830 },   // defaultFill
            juce::Colour{ 0xffffffff },   // highlightedText
            juce::Colour{ 0xff6366f1 },   // highlightedFill
            juce::Colour{ 0xffe4e4e7 },   // menuText
    })
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour{ 0xff1a1d23 });
        setColour(juce::TextEditor::backgroundColourId, juce::Colour{ 0xff101418 });
        setColour(juce::TextEditor::textColourId, juce::Colour{ 0xffd6e2ea });
        setColour(juce::TextEditor::outlineColourId, juce::Colour{ 0xff363840 });
        setColour(juce::ToggleButton::tickColourId, juce::Colour{ 0xff6366f1 });
    }
};

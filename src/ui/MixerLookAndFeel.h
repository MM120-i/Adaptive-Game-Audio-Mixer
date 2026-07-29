#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct MixerLookAndFeel final : juce::LookAndFeel_V4 {
    MixerLookAndFeel() : LookAndFeel_V4(juce::LookAndFeel_V4::ColourScheme{
            juce::Colour{0xff0f1117},
            juce::Colour{0xff1e2130},
            juce::Colour{0xff1e2130},
            juce::Colour{0xff2e3344},
            juce::Colour{0xfff1f5f9},
            juce::Colour{0xff1e2130},
            juce::Colour{0xffffffff},
            juce::Colour{0xff818cf8},
            juce::Colour{0xfff1f5f9},
    })
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour{0xff0f1117});
        setColour(juce::TextEditor::backgroundColourId, juce::Colour{0xff0a0c13});
        setColour(juce::TextEditor::textColourId, juce::Colour{0xfff1f5f9});
        setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff2e3344});
        setColour(juce::ToggleButton::tickColourId, juce::Colour{0xff818cf8});
    }
};

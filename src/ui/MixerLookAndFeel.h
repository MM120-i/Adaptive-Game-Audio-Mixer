#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct MixerLookAndFeel final : juce::LookAndFeel_V4 {
    MixerLookAndFeel() : LookAndFeel_V4(juce::LookAndFeel_V4::ColourScheme{
            juce::Colour{0xff0a0a14},
            juce::Colour{0xff12121f},
            juce::Colour{0xff12121f},
            juce::Colour{0xff1e293b},
            juce::Colour{0xfff1f5f9},
            juce::Colour{0xff12121f},
            juce::Colour{0xffffffff},
            juce::Colour{0xff06b6d4},
            juce::Colour{0xfff1f5f9},
    })
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour{0xff0a0a14});
        setColour(juce::TextEditor::backgroundColourId, juce::Colour{0xff05050d});
        setColour(juce::TextEditor::textColourId, juce::Colour{0xfff1f5f9});
        setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff1e293b});
        setColour(juce::ToggleButton::tickColourId, juce::Colour{0xff06b6d4});
    }
};

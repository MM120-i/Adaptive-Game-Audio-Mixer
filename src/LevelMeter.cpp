#include "LevelMeter.h"

void LevelMeter::setLevel(float newLevel){
    if(newLevel > decay)
        decay = newLevel;
    else
        decay *= decayRate;

    level = newLevel;
    repaint();
}

void LevelMeter::paint(juce::Graphics &g){
    const auto bounds = getLocalBounds().toFloat();
    const auto filledWith = bounds.getWidth() * decay;

    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(bounds, 4.0f);

    if(decay < 0.5f)
        g.setColour(juce::Colours::green);
    else if(decay < 0.8f)
        g.setColour(juce::Colours::yellow);
    else
        g.setColour(juce::Colours::red);

    g.fillRoundedRectangle(bounds.withWidth(filledWith), 4.0f);
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}
#include "LevelMeter.h"
#include <cmath>

namespace {
    float amplitudeToDb(float amp){
        if(amp <= 0.0f)
            return -60.0f;

        return 20.0f * std::log10(amp);
    }

    juce::Colour levelColour(float value){
        if(value < 0.5f)
            return juce::Colour::fromFloatRGBA(0.2f, 0.9f, 0.3f, 1.0f);  

        if(value < 0.8f)
            return juce::Colour::fromFloatRGBA(0.95f, 0.85f, 0.1f, 1.0f);

        return juce::Colour::fromFloatRGBA(0.95f, 0.25f, 0.15f, 1.0f);   
    }
}

void LevelMeter::setLevel(float newLevel){

    if(newLevel > decay){
        decay = newLevel;
        peakDb = amplitudeToDb(decay);
        peakHoldStart = juce::Time::getMillisecondCounterHiRes();
    }
    else{
        decay *= decayRate;
        const auto now = juce::Time::getMillisecondCounterHiRes();

        if(now - peakHoldStart > peakHoldMs)
            peakDb = -60.0f;
    }

    level = newLevel;
    repaint();
}

void LevelMeter::paint(juce::Graphics &g){
    const auto bounds = getLocalBounds().toFloat();
    const auto barHeight = bounds.getHeight() * 0.55f;
    const auto barBounds = bounds.withHeight(barHeight);
    const auto labelY = barBounds.getBottom() + 4.0f;

    g.setColour(juce::Colour{ 0xff101418 });
    g.fillRoundedRectangle(barBounds, 6.0f);

    const auto filledWidth = barBounds.getWidth() * decay;

    if(filledWidth > 1.0f){
        juce::ColourGradient gradient { 
            juce::Colour::fromFloatRGBA(0.15f, 0.85f, 0.25f, 1.0f),
            barBounds.getX(),
            barBounds.getY(),
            juce::Colour::fromFloatRGBA(0.95f, 0.2f, 0.1f, 1.0f),
            barBounds.getRight(),
            barBounds.getY(),
            false, 
        };

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(barBounds.withWidth(filledWidth), 6.0f);

        if (decay > 0.1f){
            g.setColour(levelColour(decay).withAlpha(0.4f));
            const auto glowRect = barBounds.withWidth(filledWidth);
            g.drawRoundedRectangle(glowRect.expanded(2.0f), 8.0f, 1.5f);
        }
    }

    const auto peakAmp = std::pow(10.0f, peakDb / 20.0f);
    const auto peakX = barBounds.getX() + barBounds.getWidth() * peakAmp;

    if(peakDb > -60.0f){
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawLine(peakX, barBounds.getY() - 2.0f, peakX, barBounds.getBottom() + 2.0f, 2.0f);
    }

    g.setColour(juce::Colour{ 0xff363840 });
    g.drawRoundedRectangle(barBounds, 6.0f, 1.0f);
    g.setFont(juce::FontOptions{ 11.0f });
    g.setColour(juce::Colour{ 0xff7b7d84 });

    const float dbMarks[] = { -60.0f, -40.0f, -20.0f, -10.0f, -3.0f, 0.0f };

    for(const auto db : dbMarks){
        const auto amp = std::pow(10.0f, db / 20.0f);
        const auto x = barBounds.getX() + barBounds.getWidth() * amp - 14.0f;

        juce::String label;

        if(db == 0.0f)
            label = "0";
        else
            label = juce::String(static_cast<int>(db));

        g.drawText(label, x, labelY, 28.0f, 14.0f, juce::Justification::centred);
    }

    const auto dbNow = amplitudeToDb(level);

    g.setColour(juce::Colour{ 0xffe4e4e7 });
    g.setFont(juce::FontOptions{ 12.0f, juce::Font::bold });

    g.drawText(
        juce::String (dbNow, 1) + " dB",
        barBounds.getRight() - 80.0f,
        barBounds.getY() - 18.0f,
        76.0f,
        16.0f,
        juce::Justification::centredRight
    );
}

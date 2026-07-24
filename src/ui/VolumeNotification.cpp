#include "VolumeNotification.h"

VolumeNotification::VolumeNotification(const juce::String &message)
    : TopLevelWindow("VolumeNotification", false),
      message_(message)
{
    setOpaque(false);
    setAlwaysOnTop(true);
    setUsingNativeTitleBar(false);

    auto disp = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

    if(disp){
        auto area = disp->userArea;
        setBounds(area.getX() + (area.getWidth() - width) / 2,
                  area.getY() + area.getHeight() - height - 80,
                  width, height);
    }

    setVisible(true);
    startTimer(16);
}

VolumeNotification::~VolumeNotification() = default;

void VolumeNotification::timerCallback(){
    switch(phase){
        case Phase::FADE_IN:
            phaseMs += 16;
            opacity = static_cast<float>(phaseMs) / static_cast<float>(fadeInMs);

            if(phaseMs >= fadeInMs){
                opacity = 1.0f;
                phase = Phase::HOLD;
                phaseMs = 0;
            }

            repaint();
            break;

        case Phase::HOLD:
            phaseMs += 16;

            if(phaseMs >= holdMs){
                phase = Phase::FADE_OUT;
                phaseMs = 0;
            }

            break;

        case Phase::FADE_OUT:
            phaseMs += 16;
            opacity = 1.0f - static_cast<float>(phaseMs) / static_cast<float>(fadeOutMs);

            if(phaseMs >= fadeOutMs){
                stopTimer();
                delete this;
                return;
            }

            repaint();
            break;
    }
}

void VolumeNotification::show(const juce::String &message){
    juce::MessageManager::callAsync([message]{
        new VolumeNotification(message);
    });
}

void VolumeNotification::paint(juce::Graphics &g){
    g.setOpacity(opacity);
    g.setColour(juce::Colour{0xff1a1d23});
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colour{0xffe4e4e7});
    g.setFont(juce::FontOptions{16.0f, juce::Font::bold});
    g.drawText(message_, getLocalBounds(), juce::Justification::centred);
}
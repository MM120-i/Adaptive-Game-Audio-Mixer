#include <windows.h>

#include "OverlayHud.h"
#include "core/SpotifyClient.h"
#include "ui/AudioBalancer.h"

OverlayHud::OverlayHud(SpotifyClient &sc, AudioBalancer &ab): 
    TopLevelWindow("OverlayHud", true),
    spotifyClient(sc),
    audioBalancer(ab)
{
    setAlwaysOnTop(true);
    setUsingNativeTitleBar(false);

    auto disp = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

    if(disp){
        auto area = disp->userArea;
        setBounds(area.getRight() - width - 12, area.getY() + 12, width, height);
    }

    applyNativeTweaks();
    startTimer(refreshMs);
}

OverlayHud::~OverlayHud() = default;

void OverlayHud::applyNativeTweaks(){
    auto *peer = getPeer();

    if(!peer) 
        return;

    auto hwnd = static_cast<HWND>(peer->getNativeHandle());
    auto exStyle = static_cast<LONG_PTR>(GetWindowLongPtr(hwnd, GWL_EXSTYLE));

    exStyle |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    SetLayeredWindowAttributes(hwnd, 0, 204, LWA_ALPHA);

    auto rgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, 10, 10);
    SetWindowRgn(hwnd, rgn, TRUE);

    SetWindowPos(
        hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );
}

void OverlayHud::raiseToTop(){
    auto *peer = getPeer();

    if(peer){
        auto hwnd = static_cast<HWND>(peer->getNativeHandle());
        
        SetWindowPos(
            hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
        );
    }
}

void OverlayHud::toggle(){
    toggleOn = !toggleOn;
    hideCounter = -1;
    setVisible(toggleOn);

    if(toggleOn)
        raiseToTop();
}

void OverlayHud::flash(){   
    hideCounter = 0;

    if(!isVisible()){
        setVisible(true);
        raiseToTop();
    }
}

void OverlayHud::timerCallback(){
    if(!isVisible())
        return;

    if(spotifyClient.isAuthenticated()){
        trackName = spotifyClient.trackTitle();
        artistName = spotifyClient.trackArtist();
        isPlaying = spotifyClient.isPlaying();
    }
    else{
        trackName.clear();
        artistName.clear();
        isPlaying = false;
    }

    balance = audioBalancer.getCrossFaderValue();
    repaint();

    if(!toggleOn && hideCounter >= 0){
        hideCounter += refreshMs;

        if(hideCounter >= autoHideMs){
            hideCounter = -1;
            setVisible(false);
        }
    }
}

void OverlayHud::paint(juce::Graphics &g){
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colour{0xff13161a});
    g.fillAll();
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);

    if(!spotifyClient.isAuthenticated()){
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::FontOptions{13.0f});
        g.drawText("Spotify not connected", bounds, juce::Justification::centred);

        return;
    }

    auto content = bounds.reduced(14.0f, 10.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions{13.0f, juce::Font::bold});
    g.drawText(trackName, content.removeFromTop(18.0f), juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.setFont(juce::FontOptions{11.0f});
    g.drawText(artistName, content.removeFromTop(16.0f), juce::Justification::centredLeft);

    content.removeFromTop(4.0f);

    auto barArea = content.removeFromTop(20.0f);
    auto barY = barArea.getCentreY() - 3.0f;

    g.setColour(juce::Colour{0xffe07c24});
    g.fillRoundedRectangle(barArea.getX(), barY, barArea.getWidth(), 6.0f, 3.0f);

    auto spotWidth = barArea.getWidth() * static_cast<float>(1.0 - balance);
    g.setColour(juce::Colour{0xff1db954});
    g.fillRoundedRectangle(barArea.getX(), barY, spotWidth, 6.0f, 3.0f);

    auto knobX = std::clamp(barArea.getX() + spotWidth - 4.0f, barArea.getX(), barArea.getRight() - 8.0f);
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(knobX, barArea.getCentreY() - 7.0f, 8.0f, 14.0f, 4.0f);

    auto pct = static_cast<int>((1.0 - balance) * 100);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::FontOptions{10.0f});
    g.drawText(juce::String(pct) + "%", content.removeFromTop(14.0f), juce::Justification::centred);

    auto stateText = isPlaying ? juce::String::fromUTF8("\xe2\x96\xb6  Playing")
                               : juce::String::fromUTF8("\xe2\x9d\x9a\xe2\x9d\x9a  Paused");

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::FontOptions{10.0f});
    g.drawText(stateText, content, juce::Justification::centred);
}

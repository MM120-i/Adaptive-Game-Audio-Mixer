#pragma once

#include <functional>
#include <thread>

#include "AppSettings.h"
#include <juce_core/juce_core.h>

enum class SpotifyStatus {
    Disconnected,
    Connecting, 
    Connected,
    NoActiveDevice,
    Error,
};

class SpotifyClient {
private:
    juce::String generateCodeVerifier();
    juce::String generateCodeChallenge(const juce::String &);
    void startCallbackServer();
    void exchangeCodeForTokens(const juce::String &);
    void refreshAccessToken();

    juce::String httpRequest(const juce::String &, const juce::StringArray & = {}, const juce::String &method = "GET");
    juce::var httpPostForm(const juce::String &, const juce::StringPairArray &, const juce::StringArray & = {});
    juce::String apiGet(const juce::String &);
    juce::var apiPost(const juce::String &, const juce::StringPairArray & = {}, const juce::StringArray & = {});
    juce::String apiPut(const juce::String &);

    void parsePlaybackState(const juce::var &);
    void parseDevices(const juce::var &);

    juce::String accessToken;
    juce::String refreshToken;
    juce::int64 tokenExpiry = 0;      

    juce::String currentTrack;
    juce::String currentArtist;
    juce::String currentAlbum;
    juce::String currentAlbumArtUrl;
    juce::String currentDeviceName;
    bool playing = false;
    int currentVolume = 0;
    juce::int64 lastLocalVolumeChange_ = 0;

    juce::CriticalSection lock;
    bool authenticated = false;

    juce::String codeVerifier;
    std::thread serverThread;
    std::thread pollThread;
    std::atomic<bool> pollRunning{false};
    std::atomic<bool> pollNow{false};
    uintptr_t serverSocketHandle = 0;

    SpotifyStatus status_{SpotifyStatus::Disconnected};
    juce::String lastErrorMessage_;
    bool deviceActive_ = false;

    static constexpr auto kTokenUrl = "https://accounts.spotify.com/api/token";
    static constexpr auto kApiBase = "https://api.spotify.com/v1";
    static constexpr auto kAuthorizeUrl = "https://accounts.spotify.com/authorize";
    static constexpr auto kScopes = "user-read-playback-state "
                                    "user-modify-playback-state "
                                    "user-read-currently-playing";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpotifyClient)

public:
    SpotifyClient();
    ~SpotifyClient();

    bool isAuthenticated() const;
    void startAuth();
    void disconnect();
    void startPolling();
    void stopPolling();
    void loadTokens(const AppSettings &);
    void saveTokens(AppSettings &) const;
    bool isPlaying() const;
    int  deviceVolume() const;
    void poll();
    bool hasActiveDevice() const;

    void setVolume(int);
    void setPlaying(bool);
    void skipNext();
    void skipPrevious();
    void fetchDeviceVolume();

    SpotifyStatus status() const;

    juce::String lastErrorMessage() const;
    juce::String trackTitle() const;
    juce::String trackArtist() const;
    juce::String albumName() const;
    juce::String albumArtUrl() const;
    juce::String deviceName() const;

    std::function<void()> onStateChanged;
};
